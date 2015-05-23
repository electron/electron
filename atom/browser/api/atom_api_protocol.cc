// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/adapter_request_job.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/callback.h"
#include "native_mate/dictionary.h"
#include "net/url_request/url_request_context.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace mate {

template<>
struct Converter<const net::URLRequest*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const net::URLRequest* val) {
    return mate::ObjectTemplateBuilder(isolate)
        .SetValue("method", val->method())
        .SetValue("url", val->url().spec())
        .SetValue("referrer", val->referrer())
        .Build()->NewInstance();
  }
};

}  // namespace mate


namespace atom {

namespace api {

namespace {

typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

scoped_refptr<base::RefCountedBytes> BufferToRefCountedBytes(
    v8::Local<v8::Value> buf) {
  scoped_refptr<base::RefCountedBytes> data(new base::RefCountedBytes);
  auto start = reinterpret_cast<const unsigned char*>(node::Buffer::Data(buf));
  data->data().assign(start, start + node::Buffer::Length(buf));
  return data;
}

class CustomProtocolRequestJob : public AdapterRequestJob {
 public:
  CustomProtocolRequestJob(Protocol* registry,
                           ProtocolHandler* protocol_handler,
                           net::URLRequest* request,
                           net::NetworkDelegate* network_delegate)
      : AdapterRequestJob(protocol_handler, request, network_delegate),
        registry_(registry) {
  }

  // AdapterRequestJob:
  void GetJobTypeInUI() override {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);

    // Call the JS handler.
    Protocol::JsProtocolHandler callback =
        registry_->GetProtocolHandler(request()->url().scheme());
    v8::Local<v8::Value> result = callback.Run(request());

    // Determine the type of the job we are going to create.
    if (result->IsString()) {
      std::string data = mate::V8ToString(result);
      BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
          base::Bind(&AdapterRequestJob::CreateStringJobAndStart,
                     GetWeakPtr(), "text/plain", "UTF-8", data));
      return;
    } else if (result->IsObject()) {
      v8::Local<v8::Object> obj = result->ToObject();
      mate::Dictionary dict(isolate, obj);
      std::string name = mate::V8ToString(obj->GetConstructorName());
      if (name == "RequestStringJob") {
        std::string mime_type, charset, data;
        dict.Get("mimeType", &mime_type);
        dict.Get("charset", &charset);
        dict.Get("data", &data);

        BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateStringJobAndStart,
                       GetWeakPtr(), mime_type, charset, data));
        return;
      } else if (name == "RequestBufferJob") {
        std::string mime_type, encoding;
        v8::Local<v8::Value> buffer;
        dict.Get("mimeType", &mime_type);
        dict.Get("encoding", &encoding);
        dict.Get("data", &buffer);

        BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateBufferJobAndStart,
                       GetWeakPtr(), mime_type, encoding,
                       BufferToRefCountedBytes(buffer)));
        return;
      } else if (name == "RequestFileJob") {
        base::FilePath path;
        dict.Get("path", &path);

        BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateFileJobAndStart,
                       GetWeakPtr(), path));
        return;
      } else if (name == "RequestErrorJob") {
        // Default value net::ERR_NOT_IMPLEMENTED
        int error = -11;
        dict.Get("error", &error);

        BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateErrorJobAndStart,
                       GetWeakPtr(), error));
        return;
      }
    }

    // Try the default protocol handler if we have.
    if (default_protocol_handler()) {
      BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
          base::Bind(&AdapterRequestJob::CreateJobFromProtocolHandlerAndStart,
                     GetWeakPtr()));
      return;
    }

    // Fallback to the not implemented error.
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
        base::Bind(&AdapterRequestJob::CreateErrorJobAndStart,
                   GetWeakPtr(), net::ERR_NOT_IMPLEMENTED));
  }

 private:
  Protocol* registry_;  // Weak, the Protocol class is expected to live forever.
};

// Always return the same CustomProtocolRequestJob for all requests, because
// the content API needs the ProtocolHandler to return a job immediately, and
// getting the real job from the JS requires asynchronous calls, so we have
// to create an adapter job first.
// Users can also pass an extra ProtocolHandler as the fallback one when
// registered handler doesn't want to deal with the request.
class CustomProtocolHandler : public ProtocolHandler {
 public:
  CustomProtocolHandler(api::Protocol* registry,
                        ProtocolHandler* protocol_handler = NULL)
      : registry_(registry), protocol_handler_(protocol_handler) {
  }

  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new CustomProtocolRequestJob(registry_, protocol_handler_.get(),
                                        request, network_delegate);
  }

  ProtocolHandler* ReleaseDefaultProtocolHandler() {
    return protocol_handler_.release();
  }

  ProtocolHandler* original_handler() { return protocol_handler_.get(); }

 private:
  Protocol* registry_;  // Weak, the Protocol class is expected to live forever.
  scoped_ptr<ProtocolHandler> protocol_handler_;

  DISALLOW_COPY_AND_ASSIGN(CustomProtocolHandler);
};

}  // namespace

Protocol::Protocol()
    : job_factory_(AtomBrowserContext::Get()->job_factory()) {
  CHECK(job_factory_);
}

Protocol::JsProtocolHandler Protocol::GetProtocolHandler(
    const std::string& scheme) {
  return protocol_handlers_[scheme];
}

mate::ObjectTemplateBuilder Protocol::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("registerProtocol",
                 base::Bind(&Protocol::RegisterProtocol,
                            base::Unretained(this)))
      .SetMethod("unregisterProtocol",
                 base::Bind(&Protocol::UnregisterProtocol,
                            base::Unretained(this)))
      .SetMethod("isHandledProtocol",
                 base::Bind(&Protocol::IsHandledProtocol,
                            base::Unretained(this)))
      .SetMethod("interceptProtocol",
                 base::Bind(&Protocol::InterceptProtocol,
                            base::Unretained(this)))
      .SetMethod("uninterceptProtocol",
                 base::Bind(&Protocol::UninterceptProtocol,
                            base::Unretained(this)));
}

void Protocol::RegisterProtocol(const std::string& scheme,
                                const JsProtocolHandler& callback) {
  if (ContainsKey(protocol_handlers_, scheme) ||
      job_factory_->IsHandledProtocol(scheme))
    return node::ThrowError("The scheme is already registered");

  protocol_handlers_[scheme] = callback;
  BrowserThread::PostTask(BrowserThread::IO,
                          FROM_HERE,
                          base::Bind(&Protocol::RegisterProtocolInIO,
                                     base::Unretained(this), scheme));
}

void Protocol::UnregisterProtocol(const std::string& scheme) {
  ProtocolHandlersMap::iterator it(protocol_handlers_.find(scheme));
  if (it == protocol_handlers_.end())
    return node::ThrowError("The scheme has not been registered");

  protocol_handlers_.erase(it);
  BrowserThread::PostTask(BrowserThread::IO,
                          FROM_HERE,
                          base::Bind(&Protocol::UnregisterProtocolInIO,
                                     base::Unretained(this), scheme));
}

bool Protocol::IsHandledProtocol(const std::string& scheme) {
  return job_factory_->IsHandledProtocol(scheme);
}

void Protocol::InterceptProtocol(const std::string& scheme,
                                 const JsProtocolHandler& callback) {
  if (!job_factory_->HasProtocolHandler(scheme))
    return node::ThrowError("Scheme does not exist.");

  if (ContainsKey(protocol_handlers_, scheme))
    return node::ThrowError("Cannot intercept custom procotols");

  protocol_handlers_[scheme] = callback;
  BrowserThread::PostTask(BrowserThread::IO,
                          FROM_HERE,
                          base::Bind(&Protocol::InterceptProtocolInIO,
                                     base::Unretained(this), scheme));
}

void Protocol::UninterceptProtocol(const std::string& scheme) {
  ProtocolHandlersMap::iterator it(protocol_handlers_.find(scheme));
  if (it == protocol_handlers_.end())
    return node::ThrowError("The scheme has not been registered");

  protocol_handlers_.erase(it);
  BrowserThread::PostTask(BrowserThread::IO,
                          FROM_HERE,
                          base::Bind(&Protocol::UninterceptProtocolInIO,
                                     base::Unretained(this), scheme));
}

void Protocol::RegisterProtocolInIO(const std::string& scheme) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  job_factory_->SetProtocolHandler(scheme, new CustomProtocolHandler(this));
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&Protocol::EmitEventInUI,
                                     base::Unretained(this),
                                     "registered", scheme));
}

void Protocol::UnregisterProtocolInIO(const std::string& scheme) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  job_factory_->SetProtocolHandler(scheme, NULL);
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&Protocol::EmitEventInUI,
                                     base::Unretained(this),
                                     "unregistered", scheme));
}

void Protocol::InterceptProtocolInIO(const std::string& scheme) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  ProtocolHandler* original_handler = job_factory_->GetProtocolHandler(scheme);
  if (original_handler == NULL) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &Protocol::EmitEventInUI,
        base::Unretained(this),
        "error", "There is no protocol handler to intercpet"));
    return;
  }

  job_factory_->ReplaceProtocol(
      scheme, new CustomProtocolHandler(this, original_handler));
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&Protocol::EmitEventInUI,
                                     base::Unretained(this),
                                     "intercepted", scheme));
}

void Protocol::UninterceptProtocolInIO(const std::string& scheme) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  CustomProtocolHandler* handler = static_cast<CustomProtocolHandler*>(
      job_factory_->GetProtocolHandler(scheme));
  if (handler->original_handler() == NULL) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &Protocol::EmitEventInUI,
        base::Unretained(this),
        "error", "The protocol is not intercpeted"));
    return;
  }

  // Reset the protocol handler to the orignal one and delete current protocol
  // handler.
  ProtocolHandler* original_handler = handler->ReleaseDefaultProtocolHandler();
  delete job_factory_->ReplaceProtocol(scheme, original_handler);
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&Protocol::EmitEventInUI,
                                     base::Unretained(this),
                                     "unintercepted", scheme));
}

void Protocol::EmitEventInUI(const std::string& event,
                             const std::string& parameter) {
  Emit(event, parameter);
}

// static
mate::Handle<Protocol> Protocol::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new Protocol);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("protocol", atom::api::Protocol::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_protocol, Initialize)
