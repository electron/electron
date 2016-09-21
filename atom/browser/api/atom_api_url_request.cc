// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/api/atom_api_session.h"

#include "native_mate/dictionary.h"
#include "atom/browser/net/atom_url_request.h"
#include "atom/common/node_includes.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/callback.h"

namespace {

const char* const kResponse = "response";
const char* const kData = "data";
const char* const kEnd = "end";

}
namespace mate {

template<>
struct Converter<scoped_refptr<const net::HttpResponseHeaders>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
    scoped_refptr<const net::HttpResponseHeaders> val) {

    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    if (val) {
      size_t iter = 0;
      std::string name;
      std::string value;
      while (val->EnumerateHeaderLines(&iter, &name, &value)) {
        dict.Set(name, value);
      }  
    }
    return dict.GetHandle();
  }
};

template<>
struct Converter<scoped_refptr<const net::IOBufferWithSize>> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate,
    scoped_refptr<const net::IOBufferWithSize> buffer) {
      return node::Buffer::Copy(isolate, buffer->data(), buffer->size()).ToLocalChecked();
  }
};

}
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
  auto atom_url_request = AtomURLRequest::Create(
    browser_context,
    method,
    url,
    weak_ptr);
  
  api_url_request->atom_request_ = atom_url_request;
  

  return api_url_request;
}


// static
void URLRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "URLRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
    // Request API
    .MakeDestroyable()
    .SetMethod("_write", &URLRequest::Write)
    .SetMethod("_end", &URLRequest::End)
    .SetMethod("abort", &URLRequest::Abort)
    .SetMethod("_setHeader", &URLRequest::SetHeader)
    .SetMethod("_getHeader", &URLRequest::GetHeader)
    .SetMethod("_removaHeader", &URLRequest::RemoveHeader)
    // Response APi
    .SetProperty("statusCode", &URLRequest::StatusCode)
    .SetProperty("statusMessage", &URLRequest::StatusMessage)
    .SetProperty("rawResponseHeaders", &URLRequest::RawResponseHeaders)
    .SetProperty("httpVersionMajor", &URLRequest::ResponseHttpVersionMajor)
    .SetProperty("httpVersionMinor", &URLRequest::ResponseHttpVersionMinor);
  
    
}

void URLRequest::Write() {
  atom_request_->Write();
}

void URLRequest::End() {
  pin();
  atom_request_->End();
}

void URLRequest::Abort() {
  atom_request_->Abort();
}

void URLRequest::SetHeader(const std::string& name, const std::string& value) {
  atom_request_->SetHeader(name, value);
}
std::string URLRequest::GetHeader(const std::string& name) {
  return atom_request_->GetHeader(name);
}
void URLRequest::RemoveHeader(const std::string& name) {
  atom_request_->RemoveHeader(name);
}

void URLRequest::OnAuthenticationRequired(
  scoped_refptr<const net::AuthChallengeInfo> auth_info) {
  EmitRequestEvent(
    "login",
    auth_info.get(),
    base::Bind(&AtomURLRequest::PassLoginInformation, atom_request_));
}
 

void URLRequest::OnResponseStarted() {
  EmitRequestEvent("response");
}

void URLRequest::OnResponseData(
  scoped_refptr<const net::IOBufferWithSize> buffer) {
  if (!buffer || !buffer->data() || !buffer->size()) {
    return;
  }

  EmitResponseEvent("data", buffer);
}

void URLRequest::OnResponseCompleted() {
  EmitResponseEvent("end");
}


int URLRequest::StatusCode() const {
  if (auto response_headers = atom_request_->GetResponseHeaders()) {
    return response_headers->response_code();
  }
  return -1;
}

std::string URLRequest::StatusMessage() const {
  std::string result;
  if (auto response_headers = atom_request_->GetResponseHeaders()) {
    result = response_headers->GetStatusText();
  }
  return result;
}

scoped_refptr<const net::HttpResponseHeaders> URLRequest::RawResponseHeaders() const {
	return atom_request_->GetResponseHeaders();
}

uint32_t URLRequest::ResponseHttpVersionMajor() const {
  if (auto response_headers = atom_request_->GetResponseHeaders()) {
     return response_headers->GetHttpVersion().major_value();
  }
  return 0;
}

uint32_t URLRequest::ResponseHttpVersionMinor() const {
  if (auto response_headers = atom_request_->GetResponseHeaders()) {
    return response_headers->GetHttpVersion().minor_value();
  }
  return 0;
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