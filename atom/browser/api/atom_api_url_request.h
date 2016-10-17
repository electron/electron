// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_
#define ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_

#include <array>
#include <string>
#include "atom/browser/api/event_emitter.h"
#include "atom/browser/api/trackable_object.h"
#include "base/memory/weak_ptr.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable_base.h"
#include "net/base/auth.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_context.h"

namespace atom {

class AtomURLRequest;

namespace api {

//
// The URLRequest class implements the V8 binding between the JavaScript API
// and Chromium native net library. It is responsible for handling HTTP/HTTPS
// requests.
//
// The current class provides only the binding layer. Two other JavaScript
// classes (ClientRequest and IncomingMessage) in the net module provide the
// final API, including some state management and arguments validation.
//
// URLRequest's methods fall into two main categories: command and event
// methods. They are always executed on the Browser's UI thread.
// Command methods are called directly from JavaScript code via the API defined
// in BuildPrototype. A command method is generally implemented by forwarding
// the call to a corresponding method on AtomURLRequest which does the
// synchronization on the Browser IO thread. The latter then calls into Chromium
// net library. On the other hand, net library events originate on the IO
// thread in AtomURLRequest and are synchronized back on the UI thread, then
// forwarded to a corresponding event method in URLRequest and then to
// JavaScript via the EmitRequestEvent/EmitResponseEvent helpers.
//
// URLRequest lifetime management: we followed the Wrapper/Wrappable pattern
// defined in native_mate. However, we augment that pattern with a pin/unpin
// mechanism. The main reason is that we want the JS API to provide a similar
// lifetime guarantees as the XMLHttpRequest.
// https://xhr.spec.whatwg.org/#garbage-collection
//
// The primary motivation is to not garbage collect a URLInstance as long as the
// object is emitting network events. For instance, in the following JS code
//
// (function() {
//   let request = new URLRequest(...);
//   request.on('response', (response)=>{
//    response.on('data', (data) = > {
//      console.log(data.toString());
//    });
//  });
// })();
//
// we still want data to be logged even if the response/request objects are n
// more referenced in JavaScript.
//
// Binding by simply following the native_mate Wrapper/Wrappable pattern will
// delete the URLRequest object when the corresponding JS object is collected.
// The v8 handle is a private member in WrappableBase and it is always weak,
// there is no way to make it strong without changing native_mate.
// The solution we implement consists of maintaining some kind of state that
// prevents collection of JS wrappers as long as the request is emitting network
// events. At initialization, the object is unpinned. When the request starts,
// it is pinned. When no more events would be emitted, the object is unpinned
// and lifetime is again managed by the standard native mate Wrapper/Wrappable
// pattern.
//
// pin/unpin: are implemented by constructing/reseting a V8 strong persistent
// handle.
//
// The URLRequest/AtmURLRequest interaction could have been implemented in a
// single class. However, it implies that the resulting class lifetime will be
// managed by two conflicting mechanisms: JavaScript garbage collection and
// Chromium reference counting. Reasoning about lifetime issues become much
// more complex.
//
// We chose to split the implementation into two classes linked via a
// strong/weak pointers. A URLRequest instance is deleted if it is unpinned and
// the corresponding JS wrapper object is garbage collected. On the other hand,
// an AtmURLRequest instance lifetime is totally governed by reference counting.
//
class URLRequest : public mate::EventEmitter<URLRequest> {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Methods for reporting events into JavaScript.
  void OnAuthenticationRequired(
      scoped_refptr<const net::AuthChallengeInfo> auth_info);
  void OnResponseStarted(
      scoped_refptr<net::HttpResponseHeaders> response_headers);
  void OnResponseData(scoped_refptr<const net::IOBufferWithSize> data);
  void OnResponseCompleted();
  void OnError(const std::string& error, bool isRequestError);

 protected:
  explicit URLRequest(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);
  ~URLRequest() override;

 private:
  template <typename Flags>
  class StateBase {
   public:
    void SetFlag(Flags flag);

   protected:
    explicit StateBase(Flags initialState);
    bool operator==(Flags flag) const;
    bool IsFlagSet(Flags flag) const;

   private:
    Flags state_;
  };

  enum class RequestStateFlags {
    kNotStarted = 0x0,
    kStarted = 0x1,
    kFinished = 0x2,
    kCanceled = 0x4,
    kFailed = 0x8,
    kClosed = 0x10
  };

  class RequestState : public StateBase<RequestStateFlags> {
   public:
    RequestState();
    bool NotStarted() const;
    bool Started() const;
    bool Finished() const;
    bool Canceled() const;
    bool Failed() const;
    bool Closed() const;
  };

  enum class ResponseStateFlags {
    kNotStarted = 0x0,
    kStarted = 0x1,
    kEnded = 0x2,
    kFailed = 0x4
  };

  class ResponseState : public StateBase<ResponseStateFlags> {
   public:
    ResponseState();
    bool NotStarted() const;
    bool Started() const;
    bool Ended() const;
    bool Canceled() const;
    bool Failed() const;
    bool Closed() const;
  };

  bool NotStarted() const;
  bool Finished() const;
  bool Canceled() const;
  bool Failed() const;
  bool Write(scoped_refptr<const net::IOBufferWithSize> buffer, bool is_last);
  void Cancel();
  bool SetExtraHeader(const std::string& name, const std::string& value);
  void RemoveExtraHeader(const std::string& name);
  void SetChunkedUpload(bool is_chunked_upload);

  int StatusCode() const;
  std::string StatusMessage() const;
  scoped_refptr<net::HttpResponseHeaders> RawResponseHeaders() const;
  uint32_t ResponseHttpVersionMajor() const;
  uint32_t ResponseHttpVersionMinor() const;

  template <typename... ArgTypes>
  std::array<v8::Local<v8::Value>, sizeof...(ArgTypes)> BuildArgsArray(
      ArgTypes... args) const;

  template <typename... ArgTypes>
  void EmitRequestEvent(ArgTypes... args);

  template <typename... ArgTypes>
  void EmitResponseEvent(ArgTypes... args);

  void Close();
  void pin();
  void unpin();

  scoped_refptr<AtomURLRequest> atom_request_;
  RequestState request_state_;
  ResponseState response_state_;

  // Used to implement pin/unpin.
  v8::Global<v8::Object> wrapper_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  base::WeakPtrFactory<URLRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequest);
};

template <typename... ArgTypes>
std::array<v8::Local<v8::Value>, sizeof...(ArgTypes)>
URLRequest::BuildArgsArray(ArgTypes... args) const {
  std::array<v8::Local<v8::Value>, sizeof...(ArgTypes)> result = {
      {mate::ConvertToV8(isolate(), args)...}};
  return result;
}

template <typename... ArgTypes>
void URLRequest::EmitRequestEvent(ArgTypes... args) {
  v8::HandleScope handle_scope(isolate());
  auto arguments = BuildArgsArray(args...);
  v8::Local<v8::Function> _emitRequestEvent;
  auto wrapper = GetWrapper();
  if (mate::Dictionary(isolate(), wrapper)
          .Get("_emitRequestEvent", &_emitRequestEvent))
    _emitRequestEvent->Call(wrapper, arguments.size(), arguments.data());
}

template <typename... ArgTypes>
void URLRequest::EmitResponseEvent(ArgTypes... args) {
  v8::HandleScope handle_scope(isolate());
  auto arguments = BuildArgsArray(args...);
  v8::Local<v8::Function> _emitResponseEvent;
  auto wrapper = GetWrapper();
  if (mate::Dictionary(isolate(), wrapper)
          .Get("_emitResponseEvent", &_emitResponseEvent))
    _emitResponseEvent->Call(wrapper, arguments.size(), arguments.data());
}

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_
