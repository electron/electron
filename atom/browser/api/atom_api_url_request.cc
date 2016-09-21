// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/api/atom_api_session.h"

#include "native_mate/dictionary.h"
#include "atom/browser/net/atom_url_request.h"
#include "atom/common/node_includes.h"

namespace {

const char* const kResponse = "response";
const char* const kData = "data";
const char* const kEnd = "end";

}
namespace mate {

template<>
struct Converter<scoped_refptr<net::HttpResponseHeaders>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
    scoped_refptr<net::HttpResponseHeaders> val) {

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
struct Converter<scoped_refptr<net::IOBufferWithSize>> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate,
    scoped_refptr<net::IOBufferWithSize> buffer) {
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
    .SetProperty("rawResponseHeaders", &URLRequest::RawResponseHeaders)
    .SetProperty("httpVersionMajor", &URLRequest::ResponseHttpVersionMajor)
    .SetProperty("httpVersionMinor", &URLRequest::ResponseHttpVersionMinor);
  
    
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
  //v8::Local<v8::Function> _emitResponse;

  //auto wrapper = GetWrapper();
  //if (mate::Dictionary(isolate(), wrapper).Get("_emitResponse", &_emitResponse))
  //  _emitResponse->Call(wrapper, 0, nullptr);
  EmitRequestEvent("response");
}

void URLRequest::OnResponseData(scoped_refptr<net::IOBufferWithSize> buffer) {
  if (!buffer || !buffer->data() || !buffer->size()) {
    return;
  }

  EmitResponseEvent("data", buffer);
  //v8::Local<v8::Function> _emitData;
  //auto data = mate::ConvertToV8(isolate(), buffer);

  //auto wrapper = GetWrapper();
  //if (mate::Dictionary(isolate(), wrapper).Get("_emitData", &_emitData))
  //  _emitData->Call(wrapper, 1, &data);
}

void URLRequest::OnResponseCompleted() {

  //v8::Local<v8::Function> _emitEnd;

  //auto wrapper = GetWrapper();
  //if (mate::Dictionary(isolate(), wrapper).Get("_emitEnd", &_emitEnd))
  //  _emitEnd->Call(wrapper, 0, nullptr);

  EmitResponseEvent("end");
}


int URLRequest::StatusCode() {
  if (auto response_headers = atom_url_request_->GetResponseHeaders()) {
    return response_headers->response_code();
  }
  return -1;
}

std::string URLRequest::StatusMessage() {
  std::string result;
  if (auto response_headers = atom_url_request_->GetResponseHeaders()) {
    result = response_headers->GetStatusText();
  }
  return result;
}

scoped_refptr<net::HttpResponseHeaders> URLRequest::RawResponseHeaders() {
	return atom_url_request_->GetResponseHeaders();
}

uint32_t URLRequest::ResponseHttpVersionMajor() {
  if (auto response_headers = atom_url_request_->GetResponseHeaders()) {
     return response_headers->GetHttpVersion().major_value();
  }
  return 0;
}

uint32_t URLRequest::ResponseHttpVersionMinor() {
  if (auto response_headers = atom_url_request_->GetResponseHeaders()) {
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