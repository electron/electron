// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_url_request_ns.h"

#include <utility>

#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/http/http_util.h"
#include "services/network/public/mojom/chunked_data_pipe_getter.mojom.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/native_mate_converters/gurl_converter.h"
#include "shell/common/native_mate_converters/net_converter.h"

#include "shell/common/node_includes.h"

namespace mate {

template <>
struct Converter<network::mojom::RedirectMode> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     network::mojom::RedirectMode* out) {
    std::string mode;
    if (!ConvertFromV8(isolate, val, &mode))
      return false;
    if (mode == "follow")
      *out = network::mojom::RedirectMode::kFollow;
    else if (mode == "error")
      *out = network::mojom::RedirectMode::kError;
    else if (mode == "manual")
      *out = network::mojom::RedirectMode::kManual;
    else
      return false;
    return true;
  }
};

}  // namespace mate

namespace electron {

namespace api {

namespace {

// Network state for request and response.
enum State {
  STATE_STARTED = 1 << 0,
  STATE_FINISHED = 1 << 1,
  STATE_CANCELED = 1 << 2,
  STATE_FAILED = 1 << 3,
  STATE_CLOSED = 1 << 4,
  STATE_ERROR = STATE_CANCELED | STATE_FAILED | STATE_CLOSED,
};

// Annotation tag passed to NetworkService.
const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("electron_net_module", R"(
        semantics {
          sender: "Electron Net module"
          description:
            "Issue HTTP/HTTPS requests using Chromium's native networking "
            "library."
          trigger: "Using the Net module"
          data: "Anything the user wants to send."
          destination: OTHER
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled."
        })");

}  // namespace

// Common class for streaming data.
class UploadDataPipeGetter {
 public:
  explicit UploadDataPipeGetter(URLRequestNS* request) : request_(request) {}
  virtual ~UploadDataPipeGetter() = default;

  virtual void AttachToRequestBody(network::ResourceRequestBody* body) = 0;

 protected:
  void SetCallback(network::mojom::DataPipeGetter::ReadCallback callback) {
    request_->size_callback_ = std::move(callback);
  }

  void SetPipe(mojo::ScopedDataPipeProducerHandle pipe) {
    request_->producer_ =
        std::make_unique<mojo::DataPipeProducer>(std::move(pipe));
    request_->StartWriting();
  }

 private:
  URLRequestNS* request_;

  DISALLOW_COPY_AND_ASSIGN(UploadDataPipeGetter);
};

// Streaming multipart data to NetworkService.
class MultipartDataPipeGetter : public UploadDataPipeGetter,
                                public network::mojom::DataPipeGetter {
 public:
  explicit MultipartDataPipeGetter(URLRequestNS* request)
      : UploadDataPipeGetter(request) {}
  ~MultipartDataPipeGetter() override = default;

  void AttachToRequestBody(network::ResourceRequestBody* body) override {
    network::mojom::DataPipeGetterPtr data_pipe_getter;
    binding_set_.AddBinding(this, mojo::MakeRequest(&data_pipe_getter));
    body->AppendDataPipe(std::move(data_pipe_getter));
  }

 private:
  // network::mojom::DataPipeGetter:
  void Read(mojo::ScopedDataPipeProducerHandle pipe,
            ReadCallback callback) override {
    SetCallback(std::move(callback));
    SetPipe(std::move(pipe));
  }

  void Clone(network::mojom::DataPipeGetterRequest request) override {
    binding_set_.AddBinding(this, std::move(request));
  }

  mojo::BindingSet<network::mojom::DataPipeGetter> binding_set_;
};

// Streaming chunked data to NetworkService.
class ChunkedDataPipeGetter : public UploadDataPipeGetter,
                              public network::mojom::ChunkedDataPipeGetter {
 public:
  explicit ChunkedDataPipeGetter(URLRequestNS* request)
      : UploadDataPipeGetter(request) {}
  ~ChunkedDataPipeGetter() override = default;

  void AttachToRequestBody(network::ResourceRequestBody* body) override {
    network::mojom::ChunkedDataPipeGetterPtr data_pipe_getter;
    binding_set_.AddBinding(this, mojo::MakeRequest(&data_pipe_getter));
    body->SetToChunkedDataPipe(std::move(data_pipe_getter));
  }

 private:
  // network::mojom::ChunkedDataPipeGetter:
  void GetSize(GetSizeCallback callback) override {
    SetCallback(std::move(callback));
  }

  void StartReading(mojo::ScopedDataPipeProducerHandle pipe) override {
    SetPipe(std::move(pipe));
  }

  mojo::BindingSet<network::mojom::ChunkedDataPipeGetter> binding_set_;
};

URLRequestNS::URLRequestNS(mate::Arguments* args) : weak_factory_(this) {
  request_ = std::make_unique<network::ResourceRequest>();
  mate::Dictionary dict;
  if (args->GetNext(&dict)) {
    dict.Get("method", &request_->method);
    dict.Get("url", &request_->url);
    dict.Get("redirect", &redirect_mode_);
    request_->redirect_mode = redirect_mode_;
  }

  std::string partition;
  mate::Handle<api::Session> session;
  if (!dict.Get("session", &session)) {
    if (dict.Get("partition", &partition))
      session = Session::FromPartition(args->isolate(), partition);
    else  // default session
      session = Session::FromPartition(args->isolate(), "");
  }

  auto* browser_context = session->browser_context();
  url_loader_factory_ =
      content::BrowserContext::GetDefaultStoragePartition(browser_context)
          ->GetURLLoaderFactoryForBrowserProcess();

  InitWith(args->isolate(), args->GetThis());
}

URLRequestNS::~URLRequestNS() {}

bool URLRequestNS::NotStarted() const {
  return request_state_ == 0;
}

bool URLRequestNS::Finished() const {
  return request_state_ & STATE_FINISHED;
}

void URLRequestNS::Cancel() {
  // Cancel only once.
  if (request_state_ & (STATE_CANCELED | STATE_CLOSED))
    return;

  // Mark as canceled.
  request_state_ |= STATE_CANCELED;
  EmitEvent(EventType::kRequest, true, "abort");

  if ((response_state_ & STATE_STARTED) && !(response_state_ & STATE_FINISHED))
    EmitEvent(EventType::kResponse, true, "aborted");

  Close();
}

void URLRequestNS::Close() {
  if (!(request_state_ & STATE_CLOSED)) {
    request_state_ |= STATE_CLOSED;
    if (response_state_ & STATE_STARTED) {
      // Emit a close event if we really have a response object.
      EmitEvent(EventType::kResponse, true, "close");
    }
    EmitEvent(EventType::kRequest, true, "close");
  }
  Unpin();
  loader_.reset();
}

bool URLRequestNS::Write(v8::Local<v8::Value> data, bool is_last) {
  if (request_state_ & (STATE_FINISHED | STATE_ERROR))
    return false;

  size_t length = node::Buffer::Length(data);

  if (!loader_) {
    // Pin on first write.
    request_state_ = STATE_STARTED;
    Pin();

    // Create the loader.
    network::ResourceRequest* request_ref = request_.get();
    loader_ = network::SimpleURLLoader::Create(std::move(request_),
                                               kTrafficAnnotation);
    loader_->SetOnResponseStartedCallback(base::Bind(
        &URLRequestNS::OnResponseStarted, weak_factory_.GetWeakPtr()));
    loader_->SetOnRedirectCallback(
        base::Bind(&URLRequestNS::OnRedirect, weak_factory_.GetWeakPtr()));
    loader_->SetOnUploadProgressCallback(base::Bind(
        &URLRequestNS::OnUploadProgress, weak_factory_.GetWeakPtr()));

    // Create upload data pipe if we have data to write.
    if (length > 0) {
      request_ref->request_body = new network::ResourceRequestBody();
      if (is_chunked_upload_)
        data_pipe_getter_ = std::make_unique<ChunkedDataPipeGetter>(this);
      else
        data_pipe_getter_ = std::make_unique<MultipartDataPipeGetter>(this);
      data_pipe_getter_->AttachToRequestBody(request_ref->request_body.get());
    }

    // Start downloading.
    loader_->DownloadAsStream(url_loader_factory_.get(), this);
  }

  if (length > 0)
    pending_writes_.emplace_back(node::Buffer::Data(data), length);

  if (is_last) {
    // The ElementsUploadDataStream requires the knowledge of content length
    // before doing upload, while Node's stream does not give us any size
    // information. So the only option left for us is to keep all the write
    // data in memory and flush them after the write is done.
    //
    // While this looks frustrating, it is actually the behavior of the non-
    // NetworkService implementation, and we are not breaking anything.
    if (!pending_writes_.empty()) {
      last_chunk_written_ = true;
      StartWriting();
    }

    request_state_ |= STATE_FINISHED;
    EmitEvent(EventType::kRequest, true, "finish");
  }
  return true;
}

void URLRequestNS::FollowRedirect() {
  if (request_state_ & (STATE_CANCELED | STATE_CLOSED))
    return;
  follow_redirect_ = true;
}

bool URLRequestNS::SetExtraHeader(const std::string& name,
                                  const std::string& value) {
  if (!request_)
    return false;
  if (!net::HttpUtil::IsValidHeaderName(name))
    return false;
  if (!net::HttpUtil::IsValidHeaderValue(value))
    return false;
  request_->headers.SetHeader(name, value);
  return true;
}

void URLRequestNS::RemoveExtraHeader(const std::string& name) {
  if (request_)
    request_->headers.RemoveHeader(name);
}

void URLRequestNS::SetChunkedUpload(bool is_chunked_upload) {
  if (request_)
    is_chunked_upload_ = is_chunked_upload;
}

mate::Dictionary URLRequestNS::GetUploadProgress() {
  mate::Dictionary progress = mate::Dictionary::CreateEmpty(isolate());
  if (loader_) {
    if (request_)
      progress.Set("started", false);
    else
      progress.Set("started", true);
    progress.Set("current", upload_position_);
    progress.Set("total", upload_total_);
    progress.Set("active", true);
  } else {
    progress.Set("active", false);
  }
  return progress;
}

int URLRequestNS::StatusCode() const {
  if (response_headers_)
    return response_headers_->response_code();
  return -1;
}

std::string URLRequestNS::StatusMessage() const {
  if (response_headers_)
    return response_headers_->GetStatusText();
  return "";
}

net::HttpResponseHeaders* URLRequestNS::RawResponseHeaders() const {
  return response_headers_.get();
}

uint32_t URLRequestNS::ResponseHttpVersionMajor() const {
  if (response_headers_)
    return response_headers_->GetHttpVersion().major_value();
  return 0;
}

uint32_t URLRequestNS::ResponseHttpVersionMinor() const {
  if (response_headers_)
    return response_headers_->GetHttpVersion().minor_value();
  return 0;
}

void URLRequestNS::OnDataReceived(base::StringPiece data,
                                  base::OnceClosure resume) {
  // In case we received an unexpected event from Chromium net, don't emit any
  // data event after request cancel/error/close.
  if (!(request_state_ & STATE_ERROR) && !(response_state_ & STATE_ERROR)) {
    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Value> buffer;
    auto maybe = node::Buffer::Copy(isolate(), data.data(), data.size());
    if (maybe.ToLocal(&buffer))
      Emit("data", buffer);
  }
  std::move(resume).Run();
}

void URLRequestNS::OnRetry(base::OnceClosure start_retry) {}

void URLRequestNS::OnComplete(bool success) {
  if (success) {
    // In case we received an unexpected event from Chromium net, don't emit any
    // data event after request cancel/error/close.
    if (!(request_state_ & STATE_ERROR) && !(response_state_ & STATE_ERROR)) {
      response_state_ |= STATE_FINISHED;
      Emit("end");
    }
  } else {  // failed
    // If response is started then emit response event, else emit request error.
    //
    // Error is only emitted when there is no previous failure. This is to align
    // with the behavior of non-NetworkService implementation.
    std::string error = net::ErrorToString(loader_->NetError());
    if (response_state_ & STATE_STARTED) {
      if (!(response_state_ & STATE_FAILED))
        EmitError(EventType::kResponse, error);
    } else {
      if (!(request_state_ & STATE_FAILED))
        EmitError(EventType::kRequest, error);
    }
  }

  Close();
}

void URLRequestNS::OnResponseStarted(
    const GURL& final_url,
    const network::ResourceResponseHead& response_head) {
  // Don't emit any event after request cancel.
  if (request_state_ & STATE_ERROR)
    return;

  response_headers_ = response_head.headers;
  response_state_ |= STATE_STARTED;
  Emit("response");
}

void URLRequestNS::OnRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& response_head,
    std::vector<std::string>* to_be_removed_headers) {
  if (!loader_)
    return;

  if (request_state_ & (STATE_CLOSED | STATE_CANCELED)) {
    NOTREACHED();
    Cancel();
    return;
  }

  switch (redirect_mode_) {
    case network::mojom::RedirectMode::kError:
      EmitError(
          EventType::kRequest,
          "Request cannot follow redirect with the current redirect mode");
      break;
    case network::mojom::RedirectMode::kManual:
      // When redirect mode is "manual", the user has to explicitly call the
      // FollowRedirect method to continue redirecting, otherwise the request
      // would be cancelled.
      //
      // Note that the SimpleURLLoader always calls FollowRedirect and does not
      // provide a formal way for us to cancel redirection, we have to cancel
      // the request to prevent the redirection.
      follow_redirect_ = false;
      EmitEvent(EventType::kRequest, false, "redirect",
                redirect_info.status_code, redirect_info.new_method,
                redirect_info.new_url, response_head.headers.get());
      if (!follow_redirect_)
        Cancel();
      break;
    case network::mojom::RedirectMode::kFollow:
      EmitEvent(EventType::kRequest, false, "redirect",
                redirect_info.status_code, redirect_info.new_method,
                redirect_info.new_url, response_head.headers.get());
      break;
  }
}

void URLRequestNS::OnUploadProgress(uint64_t position, uint64_t total) {
  upload_position_ = position;
  upload_total_ = total;
}

void URLRequestNS::OnWrite(MojoResult result) {
  if (result != MOJO_RESULT_OK)
    return;

  // Continue the pending writes.
  pending_writes_.pop_front();
  if (!pending_writes_.empty())
    DoWrite();
}

void URLRequestNS::DoWrite() {
  DCHECK(producer_);
  DCHECK(!pending_writes_.empty());
  producer_->Write(
      std::make_unique<mojo::StringDataSource>(
          pending_writes_.front(), mojo::StringDataSource::AsyncWritingMode::
                                       STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(&URLRequestNS::OnWrite, weak_factory_.GetWeakPtr()));
}

void URLRequestNS::StartWriting() {
  if (!last_chunk_written_ || size_callback_.is_null())
    return;

  size_t size = 0;
  for (const auto& data : pending_writes_)
    size += data.size();
  std::move(size_callback_).Run(net::OK, size);
  DoWrite();
}

void URLRequestNS::Pin() {
  if (wrapper_.IsEmpty()) {
    wrapper_.Reset(isolate(), GetWrapper());
  }
}

void URLRequestNS::Unpin() {
  wrapper_.Reset();
}

void URLRequestNS::EmitError(EventType type, base::StringPiece message) {
  if (type == EventType::kRequest)
    request_state_ |= STATE_FAILED;
  else
    response_state_ |= STATE_FAILED;
  v8::HandleScope handle_scope(isolate());
  auto error = v8::Exception::Error(mate::StringToV8(isolate(), message));
  EmitEvent(type, false, "error", error);
}

template <typename... Args>
void URLRequestNS::EmitEvent(EventType type, Args... args) {
  const char* method =
      type == EventType::kRequest ? "_emitRequestEvent" : "_emitResponseEvent";
  v8::HandleScope handle_scope(isolate());
  mate::CustomEmit(isolate(), GetWrapper(), method, args...);
}

// static
mate::WrappableBase* URLRequestNS::New(mate::Arguments* args) {
  return new URLRequestNS(args);
}

// static
void URLRequestNS::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "URLRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("write", &URLRequestNS::Write)
      .SetMethod("cancel", &URLRequestNS::Cancel)
      .SetMethod("setExtraHeader", &URLRequestNS::SetExtraHeader)
      .SetMethod("removeExtraHeader", &URLRequestNS::RemoveExtraHeader)
      .SetMethod("setChunkedUpload", &URLRequestNS::SetChunkedUpload)
      .SetMethod("followRedirect", &URLRequestNS::FollowRedirect)
      .SetMethod("getUploadProgress", &URLRequestNS::GetUploadProgress)
      .SetProperty("notStarted", &URLRequestNS::NotStarted)
      .SetProperty("finished", &URLRequestNS::Finished)
      .SetProperty("statusCode", &URLRequestNS::StatusCode)
      .SetProperty("statusMessage", &URLRequestNS::StatusMessage)
      .SetProperty("rawResponseHeaders", &URLRequestNS::RawResponseHeaders)
      .SetProperty("httpVersionMajor", &URLRequestNS::ResponseHttpVersionMajor)
      .SetProperty("httpVersionMinor", &URLRequestNS::ResponseHttpVersionMinor);
}

}  // namespace api

}  // namespace electron
