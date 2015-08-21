// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_protocol.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/net/adapter_request_job.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "content/public/browser/browser_thread.h"
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

  void GetJobTypeInUI(const Protocol::JsProtocolHandler& callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    v8::Locker locker(registry_->isolate());
    v8::HandleScope handle_scope(registry_->isolate());

    // Call the JS handler.
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
      mate::Dictionary dict(registry_->isolate(), obj);
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
        int error = net::ERR_NOT_IMPLEMENTED;
        dict.Get("error", &error);

        BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateErrorJobAndStart,
                       GetWeakPtr(), error));
        return;
      } else if (name == "RequestHttpJob") {
        GURL url;
        std::string method, referrer;
        dict.Get("url", &url);
        dict.Get("method", &method);
        dict.Get("referrer", &referrer);

        v8::Local<v8::Value> value;
        mate::Handle<Session> session;
        scoped_refptr<net::URLRequestContextGetter> request_context_getter;
        // "session" null -> pass nullptr;
        // "session" a Session object -> use passed session.
        // "session" undefined -> use current session;
        if (dict.Get("session", &session))
          request_context_getter =
              session->browser_context()->GetRequestContext();
        else if (dict.Get("session", &value) && value->IsNull())
          request_context_getter = nullptr;
        else
          request_context_getter = registry_->request_context_getter();

        BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateHttpJobAndStart, GetWeakPtr(),
                       request_context_getter, url, method, referrer));
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

  // AdapterRequestJob:
  void GetJobType() override {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&CustomProtocolRequestJob::GetJobTypeInUI,
                   base::Unretained(this),
                   registry_->GetProtocolHandler(request()->url().scheme())));
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

std::string ConvertErrorCode(int error_code) {
  switch (error_code) {
    case Protocol::ERR_SCHEME_REGISTERED:
      return "The Scheme is already registered";
    case Protocol::ERR_SCHEME_UNREGISTERED:
      return "The Scheme has not been registered";
    case Protocol::ERR_SCHEME_INTERCEPTED:
      return "There is no protocol handler to intercept";
    case Protocol::ERR_SCHEME_UNINTERCEPTED:
      return "The protocol is not intercepted";
    case Protocol::ERR_NO_SCHEME:
      return "The Scheme does not exist.";
    case Protocol::ERR_SCHEME:
      return "Cannot intercept custom protocols";
    default:
      NOTREACHED();
      return std::string();
  }
}

}  // namespace

Protocol::Protocol(AtomBrowserContext* browser_context)
    : request_context_getter_(browser_context->GetRequestContext()),
      job_factory_(browser_context->job_factory()) {
  CHECK(job_factory_);
}

Protocol::JsProtocolHandler Protocol::GetProtocolHandler(
    const std::string& scheme) {
  return protocol_handlers_[scheme];
}

void Protocol::OnIOActionCompleted(const JsCompletionCallback& callback,
                                   int error) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  if (error) {
    callback.Run(v8::Exception::Error(
        mate::StringToV8(isolate(), ConvertErrorCode(error))));
    return;
  }

  callback.Run(v8::Null(isolate()));
}

mate::ObjectTemplateBuilder Protocol::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("registerStandardSchemes", &Protocol::RegisterStandardSchemes)
      .SetMethod("isHandledProtocol", &Protocol::IsHandledProtocol)
      .SetMethod("_registerProtocol", &Protocol::RegisterProtocol)
      .SetMethod("_unregisterProtocol", &Protocol::UnregisterProtocol)
      .SetMethod("_interceptProtocol", &Protocol::InterceptProtocol)
      .SetMethod("_uninterceptProtocol", &Protocol::UninterceptProtocol);
}

void Protocol::RegisterStandardSchemes(
    const std::vector<std::string>& schemes) {
  atom::AtomBrowserClient::SetCustomSchemes(schemes);
}

void Protocol::IsHandledProtocol(const std::string& scheme,
                                 const net::CompletionCallback& callback) {
  BrowserThread::PostTaskAndReplyWithResult(BrowserThread::IO, FROM_HERE,
      base::Bind(&AtomURLRequestJobFactory::IsHandledProtocol,
                 base::Unretained(job_factory_), scheme),
      callback);
}

void Protocol::RegisterProtocol(v8::Isolate* isolate,
                                const std::string& scheme,
                                const JsProtocolHandler& handler,
                                const JsCompletionCallback& callback) {
  BrowserThread::PostTaskAndReplyWithResult(BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::RegisterProtocolInIO,
                 base::Unretained(this), scheme, handler),
      base::Bind(&Protocol::OnIOActionCompleted,
                 base::Unretained(this), callback));
}

void Protocol::UnregisterProtocol(v8::Isolate* isolate,
                                  const std::string& scheme,
                                  const JsCompletionCallback& callback) {
  BrowserThread::PostTaskAndReplyWithResult(BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::UnregisterProtocolInIO,
                 base::Unretained(this), scheme),
      base::Bind(&Protocol::OnIOActionCompleted,
                 base::Unretained(this), callback));
}

void Protocol::InterceptProtocol(v8::Isolate* isolate,
                                 const std::string& scheme,
                                 const JsProtocolHandler& handler,
                                 const JsCompletionCallback& callback) {
  BrowserThread::PostTaskAndReplyWithResult(BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::InterceptProtocolInIO,
                 base::Unretained(this), scheme, handler),
      base::Bind(&Protocol::OnIOActionCompleted,
                 base::Unretained(this), callback));
}

void Protocol::UninterceptProtocol(v8::Isolate* isolate,
                                   const std::string& scheme,
                                   const JsCompletionCallback& callback) {
  BrowserThread::PostTaskAndReplyWithResult(BrowserThread::IO, FROM_HERE,
      base::Bind(&Protocol::UninterceptProtocolInIO,
                 base::Unretained(this), scheme),
      base::Bind(&Protocol::OnIOActionCompleted,
                 base::Unretained(this), callback));
}

int Protocol::RegisterProtocolInIO(const std::string& scheme,
                                   const JsProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (ContainsKey(protocol_handlers_, scheme) ||
      job_factory_->IsHandledProtocol(scheme)) {
    return ERR_SCHEME_REGISTERED;
  }

  protocol_handlers_[scheme] = handler;
  job_factory_->SetProtocolHandler(scheme, new CustomProtocolHandler(this));

  return OK;
}

int Protocol::UnregisterProtocolInIO(const std::string& scheme) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ProtocolHandlersMap::iterator it(protocol_handlers_.find(scheme));
  if (it == protocol_handlers_.end()) {
    return ERR_SCHEME_UNREGISTERED;
  }

  protocol_handlers_.erase(it);
  job_factory_->SetProtocolHandler(scheme, NULL);

  return OK;
}

int Protocol::InterceptProtocolInIO(const std::string& scheme,
                                    const JsProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Force the request context to initialize, otherwise we might have nothing
  // to intercept.
  request_context_getter_->GetURLRequestContext();

  if (!job_factory_->HasProtocolHandler(scheme))
    return ERR_NO_SCHEME;

  if (ContainsKey(protocol_handlers_, scheme))
    return ERR_SCHEME;

  protocol_handlers_[scheme] = handler;
  ProtocolHandler* original_handler = job_factory_->GetProtocolHandler(scheme);
  if (original_handler == nullptr) {
    return ERR_SCHEME_INTERCEPTED;
  }

  job_factory_->ReplaceProtocol(
      scheme, new CustomProtocolHandler(this, original_handler));

  return OK;
}

int Protocol::UninterceptProtocolInIO(const std::string& scheme) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ProtocolHandlersMap::iterator it(protocol_handlers_.find(scheme));
  if (it == protocol_handlers_.end())
    return ERR_SCHEME_UNREGISTERED;

  protocol_handlers_.erase(it);
  CustomProtocolHandler* handler = static_cast<CustomProtocolHandler*>(
      job_factory_->GetProtocolHandler(scheme));
  if (handler->original_handler() == nullptr) {
    return ERR_SCHEME_UNINTERCEPTED;
  }

  // Reset the protocol handler to the orignal one and delete current protocol
  // handler.
  ProtocolHandler* original_handler = handler->ReleaseDefaultProtocolHandler();
  delete job_factory_->ReplaceProtocol(scheme, original_handler);

  return OK;
}

// static
mate::Handle<Protocol> Protocol::Create(
    v8::Isolate* isolate, AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new Protocol(browser_context));
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  auto browser_context = static_cast<atom::AtomBrowserContext*>(
      atom::AtomBrowserMainParts::Get()->browser_context());
  dict.Set("protocol", atom::api::Protocol::Create(isolate, browser_context));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_protocol, Initialize)
