// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_url_request_ns.h"

#include <utility>

#include "content/public/browser/storage_partition.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/native_mate_converters/gurl_converter.h"
#include "shell/common/native_mate_converters/net_converter.h"

#include "shell/common/node_includes.h"

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

// Call the optional callback with error.
void InvokeCallback(v8::Isolate* isolate,
                    base::OnceCallback<void(v8::Local<v8::Value>)> callback) {
  if (!callback.is_null())
    std::move(callback).Run(v8::Null(isolate));
}

void InvokeCallback(v8::Isolate* isolate,
                    base::OnceCallback<void(v8::Local<v8::Value>)> callback,
                    base::StringPiece error) {
  if (!callback.is_null()) {
    v8::Local<v8::String> msg = mate::StringToV8(isolate, error);
    v8::Local<v8::Value> error = v8::Exception::Error(msg);
    std::move(callback).Run(error);
  }
}

}  // namespace

// Streaming multipart data to NetworkService.
class UploadDataPipeGetter : public network::mojom::DataPipeGetter {
 public:
  explicit UploadDataPipeGetter(URLRequestNS* request) : request_(request) {}

  ~UploadDataPipeGetter() override = default;

  network::mojom::DataPipeGetterPtr GetPtr() {
    network::mojom::DataPipeGetterPtr data_pipe_getter;
    binding_set_.AddBinding(this, mojo::MakeRequest(&data_pipe_getter));
    return data_pipe_getter;
  }

 private:
  // network::mojom::DataPipeGetter:
  void Read(mojo::ScopedDataPipeProducerHandle pipe,
            ReadCallback callback) override {
    // Pass the size of all pending writes.
    size_t size = 0;
    for (const auto& write : request_->pending_writes_)
      size += write.data.size();
    std::move(callback).Run(net::OK, size);

    request_->producer_ =
        std::make_unique<mojo::StringDataPipeProducer>(std::move(pipe));
    request_->DoWrite();
  }

  void Clone(network::mojom::DataPipeGetterRequest request) override {
    binding_set_.AddBinding(this, std::move(request));
  }

  URLRequestNS* request_;
  mojo::BindingSet<network::mojom::DataPipeGetter> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(UploadDataPipeGetter);
};

// Streaming chunked data to NetworkService.
class UploadChunkedDataPipeGetter
    : public network::mojom::ChunkedDataPipeGetter {
 public:
  explicit UploadChunkedDataPipeGetter(URLRequestNS* request)
      : request_(request) {}

  ~UploadChunkedDataPipeGetter() override = default;

  network::mojom::ChunkedDataPipeGetterPtr GetPtr() {
    network::mojom::ChunkedDataPipeGetterPtr data_pipe_getter;
    binding_set_.AddBinding(this, mojo::MakeRequest(&data_pipe_getter));
    return data_pipe_getter;
  }

 private:
  // network::mojom::ChunkedDataPipeGetter:
  void GetSize(GetSizeCallback callback) override {
    // Pass the size of all pending writes.
    size_t size = 0;
    for (const auto& write : request_->pending_writes_)
      size += write.data.size();
    std::move(callback).Run(net::OK, size);
  }

  void StartReading(mojo::ScopedDataPipeProducerHandle pipe) override {
    request_->producer_ =
        std::make_unique<mojo::StringDataPipeProducer>(std::move(pipe));
    request_->DoWrite();
  }

  URLRequestNS* request_;

  mojo::BindingSet<network::mojom::ChunkedDataPipeGetter> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(UploadChunkedDataPipeGetter);
};

URLRequestNS::WriteData::WriteData(
    bool is_last,
    std::string data,
    base::OnceCallback<void(v8::Local<v8::Value>)> callback)
    : is_last(is_last), data(std::move(data)), callback(std::move(callback)) {}

URLRequestNS::WriteData::~WriteData() = default;

URLRequestNS::URLRequestNS(mate::Arguments* args) : weak_factory_(this) {
  request_ = std::make_unique<network::ResourceRequest>();
  mate::Dictionary dict;
  if (args->GetNext(&dict)) {
    dict.Get("method", &request_->method);
    dict.Get("url", &request_->url);
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
  // TODO(zcbenz): Implement this.
}

void URLRequestNS::Close() {
  // TODO(zcbenz): Implement this.
}

bool URLRequestNS::Write(v8::Local<v8::Value> data,
                         bool is_last,
                         v8::Local<v8::Value> extra) {
  if (request_state_ & (STATE_FINISHED | STATE_ERROR))
    return false;
  if (is_last)
    request_state_ |= STATE_FINISHED;

  size_t length = node::Buffer::Length(data);
  base::OnceCallback<void(v8::Local<v8::Value>)> callback;
  mate::ConvertFromV8(isolate(), extra, &callback);

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

    // Create upload data pipe if we have data to write.
    if (length > 0) {
      request_ref->request_body = new network::ResourceRequestBody();
      if (is_chunked_upload_) {
        upload_chunked_data_pipe_getter_ =
            std::make_unique<UploadChunkedDataPipeGetter>(this);
        request_ref->request_body->SetToChunkedDataPipe(
            upload_chunked_data_pipe_getter_->GetPtr());
      } else {
        upload_data_pipe_getter_ = std::make_unique<UploadDataPipeGetter>(this);
        request_ref->request_body->AppendDataPipe(
            upload_data_pipe_getter_->GetPtr());
      }
    }

    // Start downloading.
    loader_->DownloadAsStream(url_loader_factory_.get(), this);
  }

  if (length > 0) {
    pending_writes_.emplace_back(is_last,
                                 std::string(node::Buffer::Data(data), length),
                                 std::move(callback));
  } else {
    InvokeCallback(isolate(), std::move(callback));
  }
  return true;
}

void URLRequestNS::FollowRedirect() {
  if (request_state_ & (STATE_CANCELED | STATE_CLOSED))
    return;
  // TODO(zcbenz): Implement this.
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

void URLRequestNS::SetLoadFlags(int flags) {
  // TODO(zcbenz): Implement this.
}

mate::Dictionary URLRequestNS::GetUploadProgress() {
  mate::Dictionary progress = mate::Dictionary::CreateEmpty(isolate());
  // TODO(zcbenz): Implement this.
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
  // In case we received an unexpected event from Chromium net, don't emit any
  // data event after request cancel/error/close.
  if (!(request_state_ & STATE_ERROR) && !(response_state_ & STATE_ERROR)) {
    response_state_ |= STATE_FINISHED;
    Emit("end");
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
  response_state_ &= STATE_STARTED;
  Emit("response");
}

void URLRequestNS::OnWrite(MojoResult result) {
  auto& front = pending_writes_.front();
  if (result == MOJO_RESULT_OK)
    InvokeCallback(isolate(), std::move(front.callback));
  else
    InvokeCallback(isolate(), std::move(front.callback), "Write failed");

  if (front.is_last) {
    DCHECK(pending_writes_.empty());
    EmitRequestEvent(true, "finish");
  }

  // Continue the pending writes.
  pending_writes_.pop_front();
  if (!pending_writes_.empty())
    DoWrite();
}

void URLRequestNS::DoWrite() {
  DCHECK(producer_);
  DCHECK(!pending_writes_.empty());
  producer_->Write(
      pending_writes_.front().data,
      mojo::StringDataPipeProducer::AsyncWritingMode::
          STRING_STAYS_VALID_UNTIL_COMPLETION,
      base::BindOnce(&URLRequestNS::OnWrite, weak_factory_.GetWeakPtr()));
}

void URLRequestNS::Pin() {
  if (wrapper_.IsEmpty()) {
    wrapper_.Reset(isolate(), GetWrapper());
  }
}

void URLRequestNS::Unpin() {
  wrapper_.Reset();
}

template <typename... Args>
void URLRequestNS::EmitRequestEvent(Args... args) {
  v8::HandleScope handle_scope(isolate());
  mate::CustomEmit(isolate(), GetWrapper(), "_emitRequestEvent", args...);
}

template <typename... Args>
void URLRequestNS::EmitResponseEvent(Args... args) {
  v8::HandleScope handle_scope(isolate());
  mate::CustomEmit(isolate(), GetWrapper(), "_emitResponseEvent", args...);
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
      .SetMethod("_setLoadFlags", &URLRequestNS::SetLoadFlags)
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
