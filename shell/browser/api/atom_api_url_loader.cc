// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_url_loader.h"

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/chunked_data_pipe_getter.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

class BufferDataSource : public mojo::DataPipeProducer::DataSource {
 public:
  explicit BufferDataSource(base::span<char> buffer) {
    buffer_.resize(buffer.size());
    memcpy(buffer.data(), buffer_.data(), buffer_.size());
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
      memcpy(buffer.data(), &buffer_[offset], copyable_size);
      result.bytes_read = copyable_size;
    } else {
      NOTREACHED();
      result.result = MOJO_RESULT_OUT_OF_RANGE;
    }
    return result;
  }

  std::vector<char> buffer_;
};

class JSChunkedDataPipeGetter : public network::mojom::ChunkedDataPipeGetter {
 public:
  JSChunkedDataPipeGetter(
      v8::Isolate* isolate,
      v8::Local<v8::Function> body_func,
      mojo::PendingReceiver<network::mojom::ChunkedDataPipeGetter>
          chunked_data_pipe_getter)
      : isolate_(isolate), body_func_(isolate, body_func), weak_factory_(this) {
    receiver_.Bind(std::move(chunked_data_pipe_getter));
  }
  ~JSChunkedDataPipeGetter() override = default;

 private:
  // network::mojom::ChunkedDataPipeGetter:
  void GetSize(GetSizeCallback callback) override {
    size_callback_ = std::move(callback);
  }

  void StartReading(mojo::ScopedDataPipeProducerHandle pipe) override {
    data_producer_ = std::make_unique<mojo::DataPipeProducer>(std::move(pipe));
    // TODO: do i need to Post the remainder of this to UI?
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    v8::HandleScope handle_scope(isolate_);
    v8::MicrotasksScope script_scope(isolate_,
                                     v8::MicrotasksScope::kRunMicrotasks);
    auto write_chunk = gin::ConvertToV8(
        isolate_,
        base::BindRepeating(&WriteChunkThunk, weak_factory_.GetWeakPtr()));
    auto done = gin::ConvertToV8(
        isolate_, base::BindRepeating(&JSChunkedDataPipeGetter::Done,
                                      weak_factory_.GetWeakPtr()));
    v8::Local<v8::Value> argv[] = {write_chunk, done};
    node::Environment* env = node::Environment::GetCurrent(isolate_);
    auto global = env->context()->Global();
    node::MakeCallback(isolate_, global, body_func_.Get(isolate_),
                       node::arraysize(argv), argv, {0, 0});
  }

  // base::Bind can't handle binding to a method on a weak ptr with a return
  // value
  static v8::Local<v8::Promise> WriteChunkThunk(
      base::WeakPtr<JSChunkedDataPipeGetter> ptr,
      v8::Isolate* isolate,
      v8::Local<v8::Value> buffer) {
    if (ptr) {
      return ptr->WriteChunk(buffer);
    } else {
      return gin_helper::Promise<void>::RejectedPromise(
          isolate, "Writing to closed stream");
    }
  }

  v8::Local<v8::Promise> WriteChunk(v8::Local<v8::Value> buffer_val) {
    gin_helper::Promise<void> promise(isolate_);
    v8::Local<v8::Promise> handle = promise.GetHandle();
    if (!buffer_val->IsArrayBufferView()) {
      promise.RejectWithErrorMessage("Expected an ArrayBufferView");
      return handle;
    }
    auto buffer = buffer_val.As<v8::ArrayBufferView>();
    if (is_writing_) {
      promise.RejectWithErrorMessage("Only one write can be pending at a time");
      return handle;
    }
    if (!size_callback_) {
      promise.RejectWithErrorMessage("Can't write after calling done()");
      return handle;
    }
    is_writing_ = true;
    bytes_written_ += buffer->ByteLength();
    auto backing_store = buffer->Buffer()->GetBackingStore();
    data_producer_->Write(
        std::make_unique<BufferDataSource>(base::make_span(
            static_cast<char*>(backing_store->Data()) + buffer->ByteOffset(),
            buffer->ByteLength())),
        base::BindOnce(
            &JSChunkedDataPipeGetter::OnWriteChunkComplete,
            base::Unretained(
                this),  // TODO: should this be a weak ptr? what do we do with
                        // the promise if |this| goes away?
            std::move(promise)));
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
      size_callback_.Reset();
      // ... delete this?
    }
  }

  void Done() {
    if (size_callback_) {
      std::move(size_callback_).Run(net::OK, bytes_written_);
      // ... delete this?
    }
  }

  GetSizeCallback size_callback_;
  mojo::Receiver<network::mojom::ChunkedDataPipeGetter> receiver_{this};
  std::unique_ptr<mojo::DataPipeProducer> data_producer_;
  bool is_writing_;
  uint64_t bytes_written_;

  v8::Isolate* isolate_;
  v8::Global<v8::Function> body_func_;
  base::WeakPtrFactory<JSChunkedDataPipeGetter> weak_factory_;
};

#if 0

gin::Handle<SimpleURLLoaderWrapper> SimpleURLLoaderWrapper::Create(network::ResourceRequest request) {
  request.request_body = new network::ResourceRequestBody();
  mojo::PendingRemote<mojom::ChunkedDataPipeGetter> data_pipe_getter;

  // strong binding...?
  new JSChunkedDataPipeGetter(std::move(data_pipe_getter.InitWithNewPipeAndPassReceiver()));
  request.request_body.SetToChunkedDataPipe(std::move(data_pipe_getter));
  auto loader = SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);
  loader_->DownloadAsStream(url_loader_factory, this);
  // return ...
}
#endif

namespace electron {

namespace api {

namespace {
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
}

SimpleURLLoaderWrapper::SimpleURLLoaderWrapper(
    std::unique_ptr<network::SimpleURLLoader> loader,
    network::mojom::URLLoaderFactory* url_loader_factory)
    : loader_(std::move(loader)) {
  loader_->SetOnResponseStartedCallback(base::BindOnce(
      &SimpleURLLoaderWrapper::OnResponseStarted, base::Unretained(this)));
  loader_->DownloadAsStream(url_loader_factory, this);
  // TODO: pin this, to prevent getting GC'd until the request is finished
  /*
  loader_->SetOnRedirectCallback(
      const OnRedirectCallback& on_redirect_callback) = 0;
  loader_->SetOnUploadProgressCallback(
      UploadProgressCallback on_upload_progress_callback) = 0;
  loader_->SetOnDownloadProgressCallback(
      DownloadProgressCallback on_download_progress_callback) = 0;
  */
}
SimpleURLLoaderWrapper::~SimpleURLLoaderWrapper() = default;

void SimpleURLLoaderWrapper::Cancel() {
  loader_.reset();
  // This ensures that no further callbacks will be called, so there's no need
  // for additional guards.
}

// static
mate::WrappableBase* SimpleURLLoaderWrapper::New(gin::Arguments* args) {
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return nullptr;
  }
  auto request = std::make_unique<network::ResourceRequest>();
  opts.Get("method", &request->method);
  opts.Get("url", &request->url);

  network::ResourceRequest* request_ref = request.get();

  auto loader =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  v8::Local<v8::Value> body;
  if (opts.Get("body", &body)) {
    if (body->IsArrayBufferView()) {
      auto buffer_body = body.As<v8::ArrayBufferView>();
      auto backing_store = buffer_body->Buffer()->GetBackingStore();
      request_ref->request_body = network::ResourceRequestBody::CreateFromBytes(
          static_cast<char*>(backing_store->Data()) + buffer_body->ByteOffset(),
          buffer_body->ByteLength());
    } else if (body->IsFunction()) {
      auto body_func = body.As<v8::Function>();

      mojo::PendingRemote<network::mojom::ChunkedDataPipeGetter>
          data_pipe_getter;
      // TODO: strong binding...?
      new JSChunkedDataPipeGetter(
          args->isolate(), body_func,
          data_pipe_getter.InitWithNewPipeAndPassReceiver());
      request_ref->request_body->SetToChunkedDataPipe(
          std::move(data_pipe_getter));
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

  auto* ret =
      new SimpleURLLoaderWrapper(std::move(loader), url_loader_factory.get());
  ret->InitWithArgs(args);
  return ret;
}

void SimpleURLLoaderWrapper::OnDataReceived(base::StringPiece string_piece,
                                            base::OnceClosure resume) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  v8::HandleScope handle_scope(isolate());
  auto array_buffer = v8::ArrayBuffer::New(isolate(), string_piece.size());
  auto backing_store = array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), string_piece.data(), string_piece.size());
  Emit("data", array_buffer);
  std::move(resume).Run();
}
void SimpleURLLoaderWrapper::OnComplete(bool success) {
  Emit("complete", success);
}
void SimpleURLLoaderWrapper::OnRetry(base::OnceClosure start_retry) {}

void SimpleURLLoaderWrapper::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate());
  dict.Set("statusCode", response_head.headers->response_code());
  Emit("response-started", final_url, dict);
}

// static
void SimpleURLLoaderWrapper::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "SimpleURLLoaderWrapper"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("cancel", &SimpleURLLoaderWrapper::Cancel);
}

}  // namespace api

}  // namespace electron
