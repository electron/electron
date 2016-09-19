// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/api/atom_api_session.h"

#include "native_mate/dictionary.h"
#include "atom/browser/net/atom_url_request.h"


namespace atom {

namespace api {
  
URLRequest::URLRequest(v8::Isolate* isolate, 
                       v8::Local<v8::Object> wrapper)
  : weak_ptr_factory_(this) {
  InitWith(isolate, wrapper);
}

URLRequest::~URLRequest() {
}

// static
mate::WrappableBase* URLRequest::New(mate::Arguments* args) {

  v8::Local<v8::Object> options;
  args->GetNext(&options);
  mate::Dictionary dict(args->isolate(), options);
  std::string url;
  dict.Get("url", &url);
  std::string method;
  dict.Get("method", &method);
  std::string session_name;
  dict.Get("session", &session_name);

  auto session = Session::FromPartition(args->isolate(), session_name);

  auto browser_context = session->browser_context();

  //auto url_request_context_getter = browser_context->url_request_context_getter();
 // auto url_request_context = url_request_context_getter->GetURLRequestContext();

  //auto net_url_request = url_request_context->CreateRequest(GURL(url), 
                                              //          net::RequestPriority::DEFAULT_PRIORITY, 
                                               //         nullptr);
 // net_url_request->set_method(method);
   
 // auto atom_url_request = new URLRequest(args->isolate(), args->GetThis(), net_url_request.release());
  
  auto api_url_request = new URLRequest(args->isolate(), args->GetThis());
  auto weak_ptr = api_url_request->weak_ptr_factory_.GetWeakPtr();
  auto atom_url_request = AtomURLRequest::create(browser_context, url, weak_ptr);

  atom_url_request->set_method(method);
  
  api_url_request->atom_url_request_ = atom_url_request;
  

  return api_url_request;
}


// static
void URLRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "URLRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("start", &URLRequest::start);
}

void URLRequest::start() {
  pin();
  atom_url_request_->Start();
}

void URLRequest::stop() {

}
void URLRequest::OnResponseStarted() {
  Emit("response-started");
}

void URLRequest::pin() {
  if (wrapper_.IsEmpty()) {
    wrapper_.Reset(isolate(), GetWrapper());
  }
}

void URLRequest::unpin() {
  wrapper_.Reset();
}

}  // namespace mate

}  // namepsace mate