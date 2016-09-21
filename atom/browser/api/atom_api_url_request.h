// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_
#define ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_

#include <array>
#include "atom/browser/api/trackable_object.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context.h"
#include "net/http/http_response_headers.h"


namespace atom {

class AtomURLRequest;

namespace api {

class URLRequest : public mate::EventEmitter<URLRequest> {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype);

 protected:
  URLRequest(v8::Isolate* isolate, 
             v8::Local<v8::Object> wrapper);
  ~URLRequest() override;


private:
  void Write();
  void End();
  void Abort();
  void SetHeader();
  void GetHeader();
  void RemoveHeader();

  friend class AtomURLRequest;
  void OnResponseStarted();
  void OnResponseData(scoped_refptr<net::IOBufferWithSize> data);
  void OnResponseCompleted();

  int StatusCode();
  std::string StatusMessage();
  scoped_refptr<net::HttpResponseHeaders> RawResponseHeaders();
  uint32_t ResponseHttpVersionMajor();
  uint32_t ResponseHttpVersionMinor();


  template <typename ... ArgTypes>
  std::array<v8::Local<v8::Value>, sizeof...(ArgTypes)> 
  BuildArgsArray(ArgTypes... args);

  template <typename ... ArgTypes>
  void EmitRequestEvent(ArgTypes... args);

  template <typename ... ArgTypes>
  void EmitResponseEvent(ArgTypes... args);



  void pin();
  void unpin();

  scoped_refptr<AtomURLRequest> atom_url_request_;
  v8::Global<v8::Object> wrapper_;
  base::WeakPtrFactory<URLRequest> weak_ptr_factory_;
  

  DISALLOW_COPY_AND_ASSIGN(URLRequest);
};

template <typename ... ArgTypes>
std::array<v8::Local<v8::Value>, sizeof...(ArgTypes)>
URLRequest::BuildArgsArray(ArgTypes... args) {
  std::array<v8::Local<v8::Value>, sizeof...(ArgTypes)> result
    = { mate::ConvertToV8(isolate(), args)... };
  return result;
}

template <typename ... ArgTypes>
void URLRequest::EmitRequestEvent(ArgTypes... args) {
  auto arguments = BuildArgsArray(args...);
  v8::Local<v8::Function> _emitRequestEvent;
  auto wrapper = GetWrapper();
  if (mate::Dictionary(isolate(), wrapper).Get("_emitRequestEvent", &_emitRequestEvent))
    _emitRequestEvent->Call(wrapper, arguments.size(), arguments.data());
}


template <typename ... ArgTypes>
void URLRequest::EmitResponseEvent(ArgTypes... args) {
  auto arguments = BuildArgsArray(args...);
  v8::Local<v8::Function> _emitResponseEvent;
  auto wrapper = GetWrapper();
  if (mate::Dictionary(isolate(), wrapper).Get("_emitResponseEvent", &_emitResponseEvent))
    _emitResponseEvent->Call(wrapper, arguments.size(), arguments.data());
}




}  // namepsace api

}  // namepsace atom

#endif  // ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_