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
    // Request API
    .MakeDestroyable()
    .SetMethod("write", &URLRequest::Write)
    .SetMethod("end", &URLRequest::End)
    .SetMethod("abort", &URLRequest::Abort)
    .SetMethod("setHeader", &URLRequest::SetHeader)
    .SetMethod("getHeader", &URLRequest::GetHeader)
    .SetMethod("removaHeader", &URLRequest::RemoveHeader)
    // Response APi
    .SetProperty("statusCode", &URLRequest::StatusCode)
    .SetProperty("statusMessage", &URLRequest::StatusMessage)
    .SetProperty("responseHeaders", &URLRequest::ResponseHeaders)
    .SetProperty("responseHttpVersion", &URLRequest::ResponseHttpVersion);
}

void URLRequest::Write() {
  atom_url_request_->Write();
}

void URLRequest::End() {
  pin();
  atom_url_request_->End();
}

void URLRequest::Abort() {
  atom_url_request_->Abort();
}

void URLRequest::SetHeader() {
  atom_url_request_->SetHeader();
}
void URLRequest::GetHeader() {
  atom_url_request_->GetHeader();
}
void URLRequest::RemoveHeader() {
  atom_url_request_->RemoveHeader();
}


void URLRequest::OnResponseStarted() {
  v8::Local<v8::Function> _emitResponse;

  auto wrapper = GetWrapper();
  if (mate::Dictionary(isolate(), wrapper).Get("_emitResponse", &_emitResponse))
    _emitResponse->Call(wrapper, 0, nullptr);
}

void URLRequest::OnResponseData() {
  Emit("data");
}

void URLRequest::OnResponseEnd() {
  Emit("end");
}

int URLRequest::StatusCode() {
  return atom_url_request_->StatusCode();
}

void URLRequest::StatusMessage() {
  return atom_url_request_->StatusMessage();
}

void URLRequest::ResponseHeaders() {
  return atom_url_request_->ResponseHeaders();
}

void URLRequest::ResponseHttpVersion() {
  return atom_url_request_->ResponseHttpVersion();
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