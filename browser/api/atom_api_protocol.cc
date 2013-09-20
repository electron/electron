// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_protocol.h"

#include "base/stl_util.h"
#include "browser/atom_browser_context.h"
#include "browser/net/adapter_request_job.h"
#include "browser/net/atom_url_request_context_getter.h"
#include "browser/net/atom_url_request_job_factory.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

namespace {

// The protocol module object.
v8::Persistent<v8::Object> g_protocol_object;

// Registered protocol handlers.
typedef std::map<std::string, v8::Persistent<v8::Function>> HandlersMap;
static HandlersMap g_handlers;

// Emit an event for the protocol module.
void EmitEventInUI(const std::string& event, const std::string& parameter) {
  v8::HandleScope scope;

  v8::Local<v8::Value> argv[] = {
    v8::String::New(event.data(), event.size()),
    v8::String::New(parameter.data(), parameter.size()),
  };
  node::MakeCallback(g_protocol_object, "emit", arraysize(argv), argv);
}

// Convert the URLRequest object to V8 object.
v8::Handle<v8::Object> ConvertURLRequestToV8Object(
    const net::URLRequest* request) {
  v8::Local<v8::Object> obj = v8::Object::New();
  obj->Set(v8::String::New("method"),
           v8::String::New(request->method().c_str()));
  obj->Set(v8::String::New("url"),
           v8::String::New(request->url().spec().c_str()));
  obj->Set(v8::String::New("referrer"),
           v8::String::New(request->referrer().c_str()));
  return obj;
}

// Get the job factory.
AtomURLRequestJobFactory* GetRequestJobFactory() {
  return AtomBrowserContext::Get()->url_request_context_getter()->job_factory();
}

class CustomProtocolRequestJob : public AdapterRequestJob {
 public:
  CustomProtocolRequestJob(ProtocolHandler* protocol_handler,
                           net::URLRequest* request,
                           net::NetworkDelegate* network_delegate)
      : AdapterRequestJob(protocol_handler, request, network_delegate) {
  }

  // AdapterRequestJob:
  virtual void GetJobTypeInUI() OVERRIDE {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

    // Call the JS handler.
    v8::HandleScope scope;
    v8::Handle<v8::Value> argv[] = {
      ConvertURLRequestToV8Object(request()),
    };
    v8::Handle<v8::Value> result = g_handlers[request()->url().scheme()]->Call(
        v8::Context::GetCurrent()->Global(), arraysize(argv), argv);

    // Determine the type of the job we are going to create.
    if (result->IsString()) {
      std::string data = *v8::String::Utf8Value(result);
      content::BrowserThread::PostTask(
          content::BrowserThread::IO,
          FROM_HERE,
          base::Bind(&AdapterRequestJob::CreateStringJobAndStart,
                     GetWeakPtr(),
                     "text/plain",
                     "UTF-8",
                     data));
      return;
    } else if (result->IsObject()) {
      v8::Handle<v8::Object> obj = result->ToObject();
      std::string name = *v8::String::Utf8Value(obj->GetConstructorName());
      if (name == "RequestStringJob") {
        std::string mime_type = *v8::String::Utf8Value(obj->Get(
            v8::String::New("mimeType")));
        std::string charset = *v8::String::Utf8Value(obj->Get(
            v8::String::New("charset")));
        std::string data = *v8::String::Utf8Value(obj->Get(
            v8::String::New("data")));

        content::BrowserThread::PostTask(
            content::BrowserThread::IO,
            FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateStringJobAndStart,
                       GetWeakPtr(),
                       mime_type,
                       charset,
                       data));
        return;
      } else if (name == "RequestFileJob") {
        base::FilePath path = base::FilePath::FromUTF8Unsafe(
            *v8::String::Utf8Value(obj->Get(v8::String::New("path"))));

        content::BrowserThread::PostTask(
            content::BrowserThread::IO,
            FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateFileJobAndStart,
                       GetWeakPtr(),
                       path));
        return;
      }
    }

    // Try the default protocol handler if we have.
    if (default_protocol_handler()) {
      content::BrowserThread::PostTask(
          content::BrowserThread::IO,
          FROM_HERE,
          base::Bind(&AdapterRequestJob::CreateJobFromProtocolHandlerAndStart,
                     GetWeakPtr()));
      return;
    }

    // Fallback to the not implemented error.
    content::BrowserThread::PostTask(
        content::BrowserThread::IO,
        FROM_HERE,
        base::Bind(&AdapterRequestJob::CreateErrorJobAndStart,
                   GetWeakPtr(),
                   net::ERR_NOT_IMPLEMENTED));
  }
};

// Always return the same CustomProtocolRequestJob for all requests, because
// the content API needs the ProtocolHandler to return a job immediately, and
// getting the real job from the JS requires asynchronous calls, so we have
// to create an adapter job first.
// Users can also pass an extra ProtocolHandler as the fallback one when
// registered handler doesn't want to deal with the request.
class CustomProtocolHandler : public ProtocolHandler {
 public:
  explicit CustomProtocolHandler(ProtocolHandler* protocol_handler = NULL)
      : protocol_handler_(protocol_handler) {
  }

  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const OVERRIDE {
    return new CustomProtocolRequestJob(protocol_handler_.get(),
                                        request,
                                        network_delegate);
  }

  ProtocolHandler* ReleaseDefaultProtocolHandler() {
    return protocol_handler_.release();
  }

  ProtocolHandler* original_handler() { return protocol_handler_.get(); }

 private:
  scoped_ptr<ProtocolHandler> protocol_handler_;

  DISALLOW_COPY_AND_ASSIGN(CustomProtocolHandler);
};

}  // namespace

// static
v8::Handle<v8::Value> Protocol::RegisterProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));
  if (g_handlers.find(scheme) != g_handlers.end() ||
      net::URLRequest::IsHandledProtocol(scheme))
    return node::ThrowError("The scheme is already registered");

  // Store the handler in a map.
  if (!args[1]->IsFunction())
    return node::ThrowError("Handler must be a function");
  g_handlers[scheme] = v8::Persistent<v8::Function>::New(
      node::node_isolate, v8::Handle<v8::Function>::Cast(args[1]));

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&RegisterProtocolInIO, scheme));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Protocol::UnregisterProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));

  // Erase the handler from map.
  HandlersMap::iterator it(g_handlers.find(scheme));
  if (it == g_handlers.end())
    return node::ThrowError("The scheme has not been registered");
  g_handlers.erase(it);

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&UnregisterProtocolInIO, scheme));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Protocol::IsHandledProtocol(const v8::Arguments& args) {
  return v8::Boolean::New(net::URLRequest::IsHandledProtocol(
      *v8::String::Utf8Value(args[0])));
}

// static
v8::Handle<v8::Value> Protocol::InterceptProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));
  if (!GetRequestJobFactory()->HasProtocolHandler(scheme))
    return node::ThrowError("Cannot intercept procotol");

  if (ContainsKey(g_handlers, scheme))
    return node::ThrowError("Cannot intercept custom procotols");

  // Store the handler in a map.
  if (!args[1]->IsFunction())
    return node::ThrowError("Handler must be a function");
  g_handlers[scheme] = v8::Persistent<v8::Function>::New(
      node::node_isolate, v8::Handle<v8::Function>::Cast(args[1]));

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&InterceptProtocolInIO, scheme));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Protocol::UninterceptProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));

  // Erase the handler from map.
  HandlersMap::iterator it(g_handlers.find(scheme));
  if (it == g_handlers.end())
    return node::ThrowError("The scheme has not been registered");
  g_handlers.erase(it);

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&UninterceptProtocolInIO,
                                              scheme));
  return v8::Undefined();
}

// static
void Protocol::RegisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  AtomURLRequestJobFactory* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, new CustomProtocolHandler);

  content::BrowserThread::PostTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&EmitEventInUI,
                                              "registered",
                                              scheme));
}

// static
void Protocol::UnregisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  AtomURLRequestJobFactory* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, NULL);

  content::BrowserThread::PostTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&EmitEventInUI,
                                              "unregistered",
                                              scheme));
}

// static
void Protocol::InterceptProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  AtomURLRequestJobFactory* job_factory(GetRequestJobFactory());
  ProtocolHandler* original_handler = job_factory->GetProtocolHandler(scheme);
  if (original_handler == NULL) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&EmitEventInUI,
                   "error",
                   "There is no protocol handler to intercpet"));
    return;
  }

  job_factory->ReplaceProtocol(scheme,
                               new CustomProtocolHandler(original_handler));

  content::BrowserThread::PostTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&EmitEventInUI,
                                              "intercepted",
                                              scheme));
}

// static
void Protocol::UninterceptProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  AtomURLRequestJobFactory* job_factory(GetRequestJobFactory());

  // Check if the protocol handler is intercepted.
  CustomProtocolHandler* handler = static_cast<CustomProtocolHandler*>(
      job_factory->GetProtocolHandler(scheme));
  if (handler->original_handler() == NULL) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&EmitEventInUI,
                   "error",
                   "The protocol is not intercpeted"));
    return;
  }

  // Reset the protocol handler to the orignal one and delete current
  // protocol handler.
  ProtocolHandler* original_handler = handler->ReleaseDefaultProtocolHandler();
  delete job_factory->ReplaceProtocol(scheme, original_handler);

  content::BrowserThread::PostTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&EmitEventInUI,
                                              "unintercepted",
                                              scheme));
}

// static
void Protocol::Initialize(v8::Handle<v8::Object> target) {
  // Remember the protocol object, used for emitting event later.
  g_protocol_object = v8::Persistent<v8::Object>::New(
      node::node_isolate, target);

  node::SetMethod(target, "registerProtocol", RegisterProtocol);
  node::SetMethod(target, "unregisterProtocol", UnregisterProtocol);
  node::SetMethod(target, "isHandledProtocol", IsHandledProtocol);
  node::SetMethod(target, "interceptProtocol", InterceptProtocol);
  node::SetMethod(target, "uninterceptProtocol", UninterceptProtocol);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_protocol, atom::api::Protocol::Initialize)
