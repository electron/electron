// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_url_request.h"

#include <utility>

#include "base/containers/id_map.h"
#include "gin/handle.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/http/http_util.h"
#include "services/network/public/mojom/chunked_data_pipe_getter.mojom.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/object_template_builder.h"

#include "shell/common/node_includes.h"

namespace gin {

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

}  // namespace gin

namespace electron {

namespace api {

namespace {

base::IDMap<URLRequest*>& GetAllRequests() {
  static base::NoDestructor<base::IDMap<URLRequest*>> s_all_requests;
  return *s_all_requests;
}

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
  explicit UploadDataPipeGetter(URLRequest* request) : request_(request) {}
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
  URLRequest* request_;

  DISALLOW_COPY_AND_ASSIGN(UploadDataPipeGetter);
};

// Streaming multipart data to NetworkService.
class MultipartDataPipeGetter : public UploadDataPipeGetter,
                                public network::mojom::DataPipeGetter {
 public:
  explicit MultipartDataPipeGetter(URLRequest* request)
      : UploadDataPipeGetter(request) {}
  ~MultipartDataPipeGetter() override = default;

  void AttachToRequestBody(network::ResourceRequestBody* body) override {
    mojo::PendingRemote<network::mojom::DataPipeGetter> data_pipe_getter_remote;
    receivers_.Add(this,
                   data_pipe_getter_remote.InitWithNewPipeAndPassReceiver());
    body->AppendDataPipe(std::move(data_pipe_getter_remote));
  }

 private:
  // network::mojom::DataPipeGetter:
  void Read(mojo::ScopedDataPipeProducerHandle pipe,
            ReadCallback callback) override {
    SetCallback(std::move(callback));
    SetPipe(std::move(pipe));
  }

  void Clone(
      mojo::PendingReceiver<network::mojom::DataPipeGetter> receiver) override {
    receivers_.Add(this, std::move(receiver));
  }

  mojo::ReceiverSet<network::mojom::DataPipeGetter> receivers_;
};

// Streaming chunked data to NetworkService.
class ChunkedDataPipeGetter : public UploadDataPipeGetter,
                              public network::mojom::ChunkedDataPipeGetter {
 public:
  explicit ChunkedDataPipeGetter(URLRequest* request)
      : UploadDataPipeGetter(request) {}
  ~ChunkedDataPipeGetter() override = default;

  void AttachToRequestBody(network::ResourceRequestBody* body) override {
    mojo::PendingRemote<network::mojom::ChunkedDataPipeGetter>
        data_pipe_getter_remote;
    receiver_set_.Add(this,
                      data_pipe_getter_remote.InitWithNewPipeAndPassReceiver());
    body->SetToChunkedDataPipe(std::move(data_pipe_getter_remote));
  }

 private:
  // network::mojom::ChunkedDataPipeGetter:
  void GetSize(GetSizeCallback callback) override {
    SetCallback(std::move(callback));
  }

  void StartReading(mojo::ScopedDataPipeProducerHandle pipe) override {
    SetPipe(std::move(pipe));
  }

  mojo::ReceiverSet<network::mojom::ChunkedDataPipeGetter> receiver_set_;
};

URLRequest::URLRequest(gin::Arguments* args)
    : id_(GetAllRequests().Add(this)), weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  request_ = std::make_unique<network::ResourceRequest>();
  gin_helper::Dictionary dict;
  if (args->GetNext(&dict)) {
    dict.Get("method", &request_->method);
    dict.Get("url", &request_->url);
    dict.Get("redirect", &redirect_mode_);
    request_->redirect_mode = redirect_mode_;
  }

  request_->render_frame_id = id_;

  std::string partition;
  gin::Handle<api::Session> session;
  if (!dict.Get("session", &session)) {
    if (dict.Get("partition", &partition))
      session = Session::FromPartition(args->isolate(), partition);
    else  // default session
      session = Session::FromPartition(args->isolate(), "");
  }

  url_loader_factory_ = session->browser_context()->GetURLLoaderFactory();

  InitWithArgs(args);
}

URLRequest::~URLRequest() = default;

URLRequest* URLRequest::FromID(uint32_t id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return GetAllRequests().Lookup(id);
}

void URLRequest::OnAuthRequired(
    const GURL& url,
    bool first_auth_attempt,
    net::AuthChallengeInfo auth_info,
    network::mojom::URLResponseHeadPtr head,
    mojo::PendingRemote<network::mojom::AuthChallengeResponder>
        auth_challenge_responder) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  mojo::Remote<network::mojom::AuthChallengeResponder> auth_responder(
      std::move(auth_challenge_responder));
  auth_responder.set_disconnect_handler(
      base::BindOnce(&URLRequest::Cancel, weak_factory_.GetWeakPtr()));
  auto cb = base::BindOnce(
      [](mojo::Remote<network::mojom::AuthChallengeResponder> auth_responder,
         gin::Arguments* args) {
        base::string16 username_str, password_str;
        if (!args->GetNext(&username_str) || !args->GetNext(&password_str)) {
          auth_responder->OnAuthCredentials(base::nullopt);
          return;
        }
        auth_responder->OnAuthCredentials(
            net::AuthCredentials(username_str, password_str));
      },
      std::move(auth_responder));
  Emit("login", auth_info, base::AdaptCallbackForRepeating(std::move(cb)));
}

bool URLRequest::NotStarted() const {
  return request_state_ == 0;
}

bool URLRequest::Finished() const {
  return request_state_ & STATE_FINISHED;
}

void URLRequest::Cancel() {
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

void URLRequest::Close() {
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

bool URLRequest::Write(v8::Local<v8::Value> data, bool is_last) {
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
    loader_->SetOnResponseStartedCallback(
        base::Bind(&URLRequest::OnResponseStarted, weak_factory_.GetWeakPtr()));
    loader_->SetOnRedirectCallback(
        base::Bind(&URLRequest::OnRedirect, weak_factory_.GetWeakPtr()));
    loader_->SetOnUploadProgressCallback(
        base::Bind(&URLRequest::OnUploadProgress, weak_factory_.GetWeakPtr()));

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

void URLRequest::FollowRedirect() {
  if (request_state_ & (STATE_CANCELED | STATE_CLOSED))
    return;
  follow_redirect_ = true;
}

bool URLRequest::SetExtraHeader(const std::string& name,
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

void URLRequest::RemoveExtraHeader(const std::string& name) {
  if (request_)
    request_->headers.RemoveHeader(name);
}

void URLRequest::SetChunkedUpload(bool is_chunked_upload) {
  if (request_)
    is_chunked_upload_ = is_chunked_upload;
}

gin::Dictionary URLRequest::GetUploadProgress() {
  gin::Dictionary progress = gin::Dictionary::CreateEmpty(isolate());
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

int URLRequest::StatusCode() const {
  if (response_headers_)
    return response_headers_->response_code();
  return -1;
}

std::string URLRequest::StatusMessage() const {
  if (response_headers_)
    return response_headers_->GetStatusText();
  return "";
}

net::HttpResponseHeaders* URLRequest::RawResponseHeaders() const {
  return response_headers_.get();
}

uint32_t URLRequest::ResponseHttpVersionMajor() const {
  if (response_headers_)
    return response_headers_->GetHttpVersion().major_value();
  return 0;
}

uint32_t URLRequest::ResponseHttpVersionMinor() const {
  if (response_headers_)
    return response_headers_->GetHttpVersion().minor_value();
  return 0;
}

void URLRequest::OnDataReceived(base::StringPiece data,
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

void URLRequest::OnRetry(base::OnceClosure start_retry) {}

void URLRequest::OnComplete(bool success) {
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

void URLRequest::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  // Don't emit any event after request cancel.
  if (request_state_ & STATE_ERROR)
    return;

  response_headers_ = response_head.headers;
  response_state_ |= STATE_STARTED;
  Emit("response");
}

void URLRequest::OnRedirect(
    const net::RedirectInfo& redirect_info,
    const network::mojom::URLResponseHead& response_head,
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
      Cancel();
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

void URLRequest::OnUploadProgress(uint64_t position, uint64_t total) {
  upload_position_ = position;
  upload_total_ = total;
}

void URLRequest::OnWrite(MojoResult result) {
  if (result != MOJO_RESULT_OK)
    return;

  // Continue the pending writes.
  pending_writes_.pop_front();
  if (!pending_writes_.empty())
    DoWrite();
}

void URLRequest::DoWrite() {
  DCHECK(producer_);
  DCHECK(!pending_writes_.empty());
  producer_->Write(
      std::make_unique<mojo::StringDataSource>(
          pending_writes_.front(), mojo::StringDataSource::AsyncWritingMode::
                                       STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(&URLRequest::OnWrite, weak_factory_.GetWeakPtr()));
}

void URLRequest::StartWriting() {
  if (!last_chunk_written_ || size_callback_.is_null())
    return;

  size_t size = 0;
  for (const auto& data : pending_writes_)
    size += data.size();
  std::move(size_callback_).Run(net::OK, size);
  DoWrite();
}

void URLRequest::Pin() {
  if (wrapper_.IsEmpty()) {
    wrapper_.Reset(isolate(), GetWrapper());
  }
}

void URLRequest::Unpin() {
  wrapper_.Reset();
}

void URLRequest::EmitError(EventType type, base::StringPiece message) {
  if (type == EventType::kRequest)
    request_state_ |= STATE_FAILED;
  else
    response_state_ |= STATE_FAILED;
  v8::HandleScope handle_scope(isolate());
  auto error = v8::Exception::Error(gin::StringToV8(isolate(), message));
  EmitEvent(type, false, "error", error);
}

template <typename... Args>
void URLRequest::EmitEvent(EventType type, Args&&... args) {
  const char* method =
      type == EventType::kRequest ? "_emitRequestEvent" : "_emitResponseEvent";
  v8::HandleScope handle_scope(isolate());
  gin_helper::CustomEmit(isolate(), GetWrapper(), method,
                         std::forward<Args>(args)...);
}

// static
mate::WrappableBase* URLRequest::New(gin::Arguments* args) {
  return new URLRequest(args);
}

// static
void URLRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "URLRequest"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("write", &URLRequest::Write)
      .SetMethod("cancel", &URLRequest::Cancel)
      .SetMethod("setExtraHeader", &URLRequest::SetExtraHeader)
      .SetMethod("removeExtraHeader", &URLRequest::RemoveExtraHeader)
      .SetMethod("setChunkedUpload", &URLRequest::SetChunkedUpload)
      .SetMethod("followRedirect", &URLRequest::FollowRedirect)
      .SetMethod("getUploadProgress", &URLRequest::GetUploadProgress)
      .SetProperty("notStarted", &URLRequest::NotStarted)
      .SetProperty("finished", &URLRequest::Finished)
      .SetProperty("statusCode", &URLRequest::StatusCode)
      .SetProperty("statusMessage", &URLRequest::StatusMessage)
      .SetProperty("rawResponseHeaders", &URLRequest::RawResponseHeaders)
      .SetProperty("httpVersionMajor", &URLRequest::ResponseHttpVersionMajor)
      .SetProperty("httpVersionMinor", &URLRequest::ResponseHttpVersionMinor);
}

}  // namespace api

}  // namespace electron
