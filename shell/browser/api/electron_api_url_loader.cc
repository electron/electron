// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_url_loader.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/id_map.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/chunked_data_pipe_getter.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<network::mojom::HttpRawHeaderPairPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const network::mojom::HttpRawHeaderPairPtr& pair) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("key", base::ToLowerASCII(pair->key));
    dict.Set("value", pair->value);
    return dict.GetHandle();
  }
};

}  // namespace gin

namespace electron {

namespace api {

namespace {

class BufferDataSource : public mojo::DataPipeProducer::DataSource {
 public:
  explicit BufferDataSource(base::span<char> buffer) {
    buffer_.resize(buffer.size());
    memcpy(buffer_.data(), buffer.data(), buffer_.size());
  }
  ~BufferDataSource() override = default;

 private:
  // mojo::DataPipeProducer::DataSource:
  uint64_t GetLength() const override { return buffer_.size(); }
  ReadResult Read(uint64_t offset, base::span<char> buffer) override {
    ReadResult result;
    if (offset <= buffer_.size()) {
      size_t readable_size = buffer_.size() - offset;
      size_t writable_size = buffer.size();
      size_t copyable_size = std::min(readable_size, writable_size);
      if (copyable_size > 0) {
        memcpy(buffer.data(), &buffer_[offset], copyable_size);
      }
      result.bytes_read = copyable_size;
    } else {
      NOTREACHED();
      result.result = MOJO_RESULT_OUT_OF_RANGE;
    }
    return result;
  }

  std::vector<char> buffer_;
};

class JSChunkedDataPipeGetter : public gin::Wrappable<JSChunkedDataPipeGetter>,
                                public network::mojom::ChunkedDataPipeGetter {
 public:
  static gin::Handle<JSChunkedDataPipeGetter> Create(
      v8::Isolate* isolate,
      v8::Local<v8::Function> body_func,
      mojo::PendingReceiver<network::mojom::ChunkedDataPipeGetter>
          chunked_data_pipe_getter) {
    return gin::CreateHandle(
        isolate, new JSChunkedDataPipeGetter(
                     isolate, body_func, std::move(chunked_data_pipe_getter)));
  }

  // gin::Wrappable
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<JSChunkedDataPipeGetter>::GetObjectTemplateBuilder(
               isolate)
        .SetMethod("write", &JSChunkedDataPipeGetter::WriteChunk)
        .SetMethod("done", &JSChunkedDataPipeGetter::Done);
  }

  static gin::WrapperInfo kWrapperInfo;
  ~JSChunkedDataPipeGetter() override = default;

 private:
  JSChunkedDataPipeGetter(
      v8::Isolate* isolate,
      v8::Local<v8::Function> body_func,
      mojo::PendingReceiver<network::mojom::ChunkedDataPipeGetter>
          chunked_data_pipe_getter)
      : isolate_(isolate), body_func_(isolate, body_func) {
    receiver_.Bind(std::move(chunked_data_pipe_getter));
  }

  // network::mojom::ChunkedDataPipeGetter:
  void GetSize(GetSizeCallback callback) override {
    size_callback_ = std::move(callback);
  }

  void StartReading(mojo::ScopedDataPipeProducerHandle pipe) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    if (body_func_.IsEmpty()) {
      LOG(ERROR) << "Tried to read twice from a JSChunkedDataPipeGetter";
      // Drop the handle on the floor.
      return;
    }
    data_producer_ = std::make_unique<mojo::DataPipeProducer>(std::move(pipe));

    v8::HandleScope handle_scope(isolate_);
    v8::MicrotasksScope script_scope(isolate_,
                                     v8::MicrotasksScope::kRunMicrotasks);
    auto maybe_wrapper = GetWrapper(isolate_);
    v8::Local<v8::Value> wrapper;
    if (!maybe_wrapper.ToLocal(&wrapper)) {
      return;
    }
    v8::Local<v8::Value> argv[] = {wrapper};
    node::Environment* env = node::Environment::GetCurrent(isolate_);
    auto global = env->context()->Global();
    node::MakeCallback(isolate_, global, body_func_.Get(isolate_),
                       node::arraysize(argv), argv, {0, 0});
  }

  v8::Local<v8::Promise> WriteChunk(v8::Local<v8::Value> buffer_val) {
    gin_helper::Promise<void> promise(isolate_);
    v8::Local<v8::Promise> handle = promise.GetHandle();
    if (!buffer_val->IsArrayBufferView()) {
      promise.RejectWithErrorMessage("Expected an ArrayBufferView");
      return handle;
    }
    if (is_writing_) {
      promise.RejectWithErrorMessage("Only one write can be pending at a time");
      return handle;
    }
    if (!size_callback_) {
      promise.RejectWithErrorMessage("Can't write after calling done()");
      return handle;
    }
    auto buffer = buffer_val.As<v8::ArrayBufferView>();
    is_writing_ = true;
    bytes_written_ += buffer->ByteLength();
    auto backing_store = buffer->Buffer()->GetBackingStore();
    auto buffer_span = base::make_span(
        static_cast<char*>(backing_store->Data()) + buffer->ByteOffset(),
        buffer->ByteLength());
    auto buffer_source = std::make_unique<BufferDataSource>(buffer_span);
    data_producer_->Write(
        std::move(buffer_source),
        base::BindOnce(&JSChunkedDataPipeGetter::OnWriteChunkComplete,
                       // We're OK to use Unretained here because we own
                       // |data_producer_|.
                       base::Unretained(this), std::move(promise)));
    return handle;
  }

  void OnWriteChunkComplete(gin_helper::Promise<void> promise,
                            MojoResult result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    is_writing_ = false;
    if (result == MOJO_RESULT_OK) {
      promise.Resolve();
    } else {
      promise.RejectWithErrorMessage("mojo result not ok");
      Finished();
    }
  }

  // TODO(nornagon): accept a net error here to allow the data provider to
  // cancel the request with an error.
  void Done() {
    if (size_callback_) {
      std::move(size_callback_).Run(net::OK, bytes_written_);
      Finished();
    }
  }

  void Finished() {
    size_callback_.Reset();
    body_func_.Reset();
    receiver_.reset();
    data_producer_.reset();
  }

  GetSizeCallback size_callback_;
  mojo::Receiver<network::mojom::ChunkedDataPipeGetter> receiver_{this};
  std::unique_ptr<mojo::DataPipeProducer> data_producer_;
  bool is_writing_ = false;
  uint64_t bytes_written_ = 0;

  v8::Isolate* isolate_;
  v8::Global<v8::Function> body_func_;
};

gin::WrapperInfo JSChunkedDataPipeGetter::kWrapperInfo = {
    gin::kEmbedderNativeGin};

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

base::IDMap<SimpleURLLoaderWrapper*>& GetAllRequests() {
  static base::NoDestructor<base::IDMap<SimpleURLLoaderWrapper*>>
      s_all_requests;
  return *s_all_requests;
}

}  // namespace

gin::WrapperInfo SimpleURLLoaderWrapper::kWrapperInfo = {
    gin::kEmbedderNativeGin};

SimpleURLLoaderWrapper::SimpleURLLoaderWrapper(
    std::unique_ptr<network::ResourceRequest> request,
    network::mojom::URLLoaderFactory* url_loader_factory)
    : id_(GetAllRequests().Add(this)) {
  // We slightly abuse the |render_frame_id| field in ResourceRequest so that
  // we can correlate any authentication events that arrive with this request.
  request->render_frame_id = id_;

  // SimpleURLLoader wants to control the request body itself. We have other
  // ideas.
  auto request_body = std::move(request->request_body);
  auto* request_ref = request.get();
  loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);
  if (request_body) {
    request_ref->request_body = std::move(request_body);
  }

  loader_->SetAllowHttpErrorResults(true);
  loader_->SetOnResponseStartedCallback(base::BindOnce(
      &SimpleURLLoaderWrapper::OnResponseStarted, base::Unretained(this)));
  loader_->SetOnRedirectCallback(base::BindRepeating(
      &SimpleURLLoaderWrapper::OnRedirect, base::Unretained(this)));
  loader_->SetOnUploadProgressCallback(base::BindRepeating(
      &SimpleURLLoaderWrapper::OnUploadProgress, base::Unretained(this)));
  loader_->SetOnDownloadProgressCallback(base::BindRepeating(
      &SimpleURLLoaderWrapper::OnDownloadProgress, base::Unretained(this)));

  loader_->DownloadAsStream(url_loader_factory, this);
}

void SimpleURLLoaderWrapper::Pin() {
  // Prevent ourselves from being GC'd until the request is complete.  Must be
  // called after gin::CreateHandle, otherwise the wrapper isn't initialized.
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  pinned_wrapper_.Reset(isolate, GetWrapper(isolate).ToLocalChecked());
}

void SimpleURLLoaderWrapper::PinBodyGetter(v8::Local<v8::Value> body_getter) {
  pinned_chunk_pipe_getter_.Reset(v8::Isolate::GetCurrent(), body_getter);
}

SimpleURLLoaderWrapper::~SimpleURLLoaderWrapper() {
  GetAllRequests().Remove(id_);
}

// static
SimpleURLLoaderWrapper* SimpleURLLoaderWrapper::FromID(uint32_t id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return GetAllRequests().Lookup(id);
}

void SimpleURLLoaderWrapper::OnAuthRequired(
    const GURL& url,
    bool first_auth_attempt,
    net::AuthChallengeInfo auth_info,
    network::mojom::URLResponseHeadPtr head,
    mojo::PendingRemote<network::mojom::AuthChallengeResponder>
        auth_challenge_responder) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  mojo::Remote<network::mojom::AuthChallengeResponder> auth_responder(
      std::move(auth_challenge_responder));
  // WeakPtr because if we're Cancel()ed while waiting for auth, and the
  // network service also decides to cancel at the same time and kill this
  // pipe, we might end up trying to call Cancel again on dead memory.
  auth_responder.set_disconnect_handler(base::BindOnce(
      &SimpleURLLoaderWrapper::Cancel, weak_factory_.GetWeakPtr()));
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

void SimpleURLLoaderWrapper::Cancel() {
  loader_.reset();
  pinned_wrapper_.Reset();
  pinned_chunk_pipe_getter_.Reset();
  // This ensures that no further callbacks will be called, so there's no need
  // for additional guards.
}

// static
gin::Handle<SimpleURLLoaderWrapper> SimpleURLLoaderWrapper::Create(
    gin::Arguments* args) {
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return gin::Handle<SimpleURLLoaderWrapper>();
  }
  auto request = std::make_unique<network::ResourceRequest>();
  request->force_ignore_site_for_cookies = true;
  opts.Get("method", &request->method);
  opts.Get("url", &request->url);
  opts.Get("referrer", &request->referrer);
  std::map<std::string, std::string> extra_headers;
  if (opts.Get("extraHeaders", &extra_headers)) {
    for (const auto& it : extra_headers) {
      if (!net::HttpUtil::IsValidHeaderName(it.first) ||
          !net::HttpUtil::IsValidHeaderValue(it.second)) {
        args->ThrowTypeError("Invalid header name or value");
        return gin::Handle<SimpleURLLoaderWrapper>();
      }
      request->headers.SetHeader(it.first, it.second);
    }
  }

  bool use_session_cookies = false;
  opts.Get("useSessionCookies", &use_session_cookies);
  if (!use_session_cookies) {
    request->load_flags |= net::LOAD_DO_NOT_SEND_COOKIES;
  }

  // Chromium filters headers using browser rules, while for net module we have
  // every header passed.
  request->report_raw_headers = true;

  v8::Local<v8::Value> body;
  v8::Local<v8::Value> chunk_pipe_getter;
  if (opts.Get("body", &body)) {
    if (body->IsArrayBufferView()) {
      auto buffer_body = body.As<v8::ArrayBufferView>();
      auto backing_store = buffer_body->Buffer()->GetBackingStore();
      request->request_body = network::ResourceRequestBody::CreateFromBytes(
          static_cast<char*>(backing_store->Data()) + buffer_body->ByteOffset(),
          buffer_body->ByteLength());
    } else if (body->IsFunction()) {
      auto body_func = body.As<v8::Function>();

      mojo::PendingRemote<network::mojom::ChunkedDataPipeGetter>
          data_pipe_getter;
      chunk_pipe_getter = JSChunkedDataPipeGetter::Create(
                              args->isolate(), body_func,
                              data_pipe_getter.InitWithNewPipeAndPassReceiver())
                              .ToV8();
      request->request_body = new network::ResourceRequestBody();
      request->request_body->SetToChunkedDataPipe(std::move(data_pipe_getter));
    }
  }

  std::string partition;
  gin::Handle<Session> session;
  if (!opts.Get("session", &session)) {
    if (opts.Get("partition", &partition))
      session = Session::FromPartition(args->isolate(), partition);
    else  // default session
      session = Session::FromPartition(args->isolate(), "");
  }

  auto url_loader_factory = session->browser_context()->GetURLLoaderFactory();

  auto ret = gin::CreateHandle(
      args->isolate(),
      new SimpleURLLoaderWrapper(std::move(request), url_loader_factory.get()));
  ret->Pin();
  if (!chunk_pipe_getter.IsEmpty()) {
    ret->PinBodyGetter(chunk_pipe_getter);
  }
  return ret;
}

void SimpleURLLoaderWrapper::OnDataReceived(base::StringPiece string_piece,
                                            base::OnceClosure resume) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  auto array_buffer = v8::ArrayBuffer::New(isolate, string_piece.size());
  auto backing_store = array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), string_piece.data(), string_piece.size());
  Emit("data", array_buffer);
  std::move(resume).Run();
}

void SimpleURLLoaderWrapper::OnComplete(bool success) {
  if (success) {
    Emit("complete");
  } else {
    Emit("error", net::ErrorToString(loader_->NetError()));
  }
  loader_.reset();
  pinned_wrapper_.Reset();
  pinned_chunk_pipe_getter_.Reset();
}

void SimpleURLLoaderWrapper::OnRetry(base::OnceClosure start_retry) {}

void SimpleURLLoaderWrapper::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(isolate);
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("statusCode", response_head.headers->response_code());
  dict.Set("statusMessage", response_head.headers->GetStatusText());
  dict.Set("httpVersion", response_head.headers->GetHttpVersion());
  // Note that |response_head.headers| are filtered by Chromium and should not
  // be used here.
  DCHECK(response_head.raw_request_response_info);
  dict.Set("rawHeaders",
           response_head.raw_request_response_info->response_headers);
  Emit("response-started", final_url, dict);
}

void SimpleURLLoaderWrapper::OnRedirect(
    const net::RedirectInfo& redirect_info,
    const network::mojom::URLResponseHead& response_head,
    std::vector<std::string>* removed_headers) {
  Emit("redirect", redirect_info, response_head.headers.get());
}

void SimpleURLLoaderWrapper::OnUploadProgress(uint64_t position,
                                              uint64_t total) {
  Emit("upload-progress", position, total);
}

void SimpleURLLoaderWrapper::OnDownloadProgress(uint64_t current) {
  Emit("download-progress", current);
}

// static
gin::ObjectTemplateBuilder SimpleURLLoaderWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             SimpleURLLoaderWrapper>::GetObjectTemplateBuilder(isolate)
      .SetMethod("cancel", &SimpleURLLoaderWrapper::Cancel);
}

const char* SimpleURLLoaderWrapper::GetTypeName() {
  return "SimpleURLLoaderWrapper";
}

}  // namespace api

}  // namespace electron
