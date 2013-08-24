// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_protocol.h"

#include "browser/atom_browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "vendor/node/src/node.h"

namespace atom {

namespace api {

namespace {

net::URLRequestJobFactoryImpl* GetRequestJobFactory() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  // Get the job factory.
  net::URLRequestJobFactoryImpl* job_factory =
      static_cast<net::URLRequestJobFactoryImpl*>(
          const_cast<net::URLRequestJobFactory*>(
              static_cast<content::BrowserContext*>(AtomBrowserContext::Get())->
                  GetRequestContext()->GetURLRequestContext()->job_factory()));
  return job_factory;
}

class AdapterProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  AdapterProtocolHandler() {}
  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const OVERRIDE {
    return NULL;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AdapterProtocolHandler);
};

}  // namespace

Protocol::HandlersMap Protocol::handlers_;

// static
v8::Handle<v8::Value> Protocol::RegisterProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));
  if (handlers_.find(scheme) != handlers_.end())
    return node::ThrowError("The scheme is already registered");

  // Store the handler in a map.
  if (!args[1]->IsFunction())
    return node::ThrowError("Handler must be a function");
  handlers_[scheme] = v8::Persistent<v8::Function>::New(
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
  HandlersMap::iterator it(handlers_.find(scheme));
  if (it == handlers_.end())
    return node::ThrowError("The scheme has not been registered");
  handlers_.erase(it);

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE, 
                                   base::Bind(&UnregisterProtocolInIO, scheme));

  return v8::Undefined();
}

// static
void Protocol::RegisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  net::URLRequestJobFactoryImpl* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, new AdapterProtocolHandler);
}

// static
void Protocol::UnregisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  net::URLRequestJobFactoryImpl* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, NULL);
}

// static
void Protocol::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "registerProtocol", RegisterProtocol);
  node::SetMethod(target, "unregisterProtocol", UnregisterProtocol);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_protocol, atom::api::Protocol::Initialize)
