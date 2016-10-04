// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/net/atom_url_request.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"



namespace mate {

template<>
struct Converter<scoped_refptr<const net::HttpResponseHeaders>> {
  static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate,
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
    return node::Buffer::Copy(isolate,
      buffer->data(),
      buffer->size()).ToLocalChecked();
  }

  static bool FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    scoped_refptr<const net::IOBufferWithSize>* out) {
    auto size = node::Buffer::Length(val);

    if (size == 0) {
      // Support conversion from empty buffer. A use case is
      // a GET request without body.
      // Since zero-sized IOBuffer(s) are not supported, we set the
      // out pointer to null.
      *out = nullptr;
      return true;
    }
    auto data = node::Buffer::Data(val);
    if (!data) {
      // This is an error as size is positive but data is null.
      return false;
    }

    auto io_buffer = new net::IOBufferWithSize(size);
    if (!io_buffer) {
      // Assuming allocation failed.
      return false;
    }

    // We do a deep copy. We could have used Buffer's internal memory
    // but that is much more complicated to be properly handled.
    memcpy(io_buffer->data(), data, size);
    *out = io_buffer;
    return true;
  }
};

}  // namespace mate

namespace atom {
namespace api {


template <typename Flags>
URLRequest::StateBase<Flags>::StateBase(Flags initialState)
  : state_(initialState) {
}

template <typename Flags>
void URLRequest::StateBase<Flags>::SetFlag(Flags flag) {
  state_ = static_cast<Flags>(static_cast<int>(state_) &
    static_cast<int>(flag));
}

template <typename Flags>
bool URLRequest::StateBase<Flags>::operator==(Flags flag) const {
  return state_ == flag;
}

template <typename Flags>
bool URLRequest::StateBase<Flags>::IsFlagSet(Flags flag) const {
  return static_cast<int>(state_) & static_cast<int>(flag);
}

URLRequest::RequestState::RequestState()
  : StateBase(RequestStateFlags::kNotStarted) {
}

bool URLRequest::RequestState::NotStarted() const {
  return *this == RequestStateFlags::kNotStarted;
}

bool URLRequest::RequestState::Started() const {
  return IsFlagSet(RequestStateFlags::kStarted);
}

bool URLRequest::RequestState::Finished() const {
  return IsFlagSet(RequestStateFlags::kFinished);
}

bool URLRequest::RequestState::Canceled() const {
  return IsFlagSet(RequestStateFlags::kCanceled);
}

bool URLRequest::RequestState::Failed() const {
  return IsFlagSet(RequestStateFlags::kFailed);
}

bool URLRequest::RequestState::Closed() const {
  return IsFlagSet(RequestStateFlags::kClosed);
}

URLRequest::ResponseState::ResponseState()
  : StateBase(ResponseStateFlags::kNotStarted) {
}

bool URLRequest::ResponseState::NotStarted() const {
  return *this == ResponseStateFlags::kNotStarted;
}

bool URLRequest::ResponseState::Started() const {
  return IsFlagSet(ResponseStateFlags::kStarted);
}

bool URLRequest::ResponseState::Ended() const {
  return IsFlagSet(ResponseStateFlags::kEnded);
}


bool URLRequest::ResponseState::Failed() const {
  return IsFlagSet(ResponseStateFlags::kFailed);
}

URLRequest::URLRequest(v8::Isolate* isolate, v8::Local<v8::Object> wrapper)
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
  std::string method;
  dict.Get("method", &method);
  std::string url;
  dict.Get("url", &url);
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
    .SetMethod("write", &URLRequest::Write)
    .SetMethod("cancel", &URLRequest::Cancel)
    .SetMethod("setExtraHeader", &URLRequest::SetExtraHeader)
    .SetMethod("removeExtraHeader", &URLRequest::RemoveExtraHeader)
    .SetMethod("setChunkedUpload", &URLRequest::SetChunkedUpload)
    .SetProperty("notStarted", &URLRequest::NotStarted)
    .SetProperty("finished", &URLRequest::Finished)
    // Response APi
    .SetProperty("statusCode", &URLRequest::StatusCode)
    .SetProperty("statusMessage", &URLRequest::StatusMessage)
    .SetProperty("rawResponseHeaders", &URLRequest::RawResponseHeaders)
    .SetProperty("httpVersionMajor", &URLRequest::ResponseHttpVersionMajor)
    .SetProperty("httpVersionMinor", &URLRequest::ResponseHttpVersionMinor);
}

bool URLRequest::NotStarted() const {
  return request_state_.NotStarted();
}

bool URLRequest::Finished() const {
  return request_state_.Finished();
}

bool URLRequest::Canceled() const {
  return request_state_.Canceled();
}

bool URLRequest::Write(
    scoped_refptr<const net::IOBufferWithSize> buffer,
    bool is_last) {
  if (request_state_.Canceled() ||
    request_state_.Failed() ||
    request_state_.Finished() ||
    request_state_.Closed()) {
    return false;
  }

  if (request_state_.NotStarted()) {
    request_state_.SetFlag(RequestStateFlags::kStarted);
    // Pin on first write.
    pin();
  }

  if (is_last) {
    request_state_.SetFlag(RequestStateFlags::kFinished);
    EmitRequestEvent(true, "finish");
  }

  DCHECK(atom_request_);
  if (atom_request_) {
    return atom_request_->Write(buffer, is_last);
  }
  return false;
}


void URLRequest::Cancel() {
  if (request_state_.Canceled() ||
      request_state_.Closed()) {
    // Cancel only once.
    return;
  }

  // Mark as canceled.
  request_state_.SetFlag(RequestStateFlags::kCanceled);

  DCHECK(atom_request_);
  if (atom_request_ && request_state_.Started()) {
    // Really cancel if it was started.
    atom_request_->Cancel();
  }
  EmitRequestEvent(true, "abort");

  if (response_state_.Started() && !response_state_.Ended()) {
    EmitResponseEvent(true, "aborted");
  }
  Close();
}

bool URLRequest::SetExtraHeader(const std::string& name,
                           const std::string& value) {
  // State must be equal to not started.
  if (!request_state_.NotStarted()) {
    // Cannot change headers after send.
    return false;
  }

  if (!net::HttpUtil::IsValidHeaderName(name)) {
    return false;
  }

  if (!net::HttpUtil::IsValidHeaderValue(value)) {
    return false;
  }

  atom_request_->SetExtraHeader(name, value);
  return true;
}

void URLRequest::RemoveExtraHeader(const std::string& name) {
  // State must be equal to not started.
  if (!request_state_.NotStarted()) {
    // Cannot change headers after send.
    return;
  }
  atom_request_->RemoveExtraHeader(name);
}

void URLRequest::SetChunkedUpload(bool is_chunked_upload) {
  // State must be equal to not started.
  if (!request_state_.NotStarted()) {
    // Cannot change headers after send.
    return;
  }
  atom_request_->SetChunkedUpload(is_chunked_upload);
}

void URLRequest::OnAuthenticationRequired(
    scoped_refptr<const net::AuthChallengeInfo> auth_info) {
  EmitRequestEvent(
    false,
    "login",
    auth_info.get(),
    base::Bind(&AtomURLRequest::PassLoginInformation, atom_request_));
}

void URLRequest::OnResponseStarted(
    scoped_refptr<const net::HttpResponseHeaders> response_headers) {
  if (request_state_.Canceled() ||
      request_state_.Failed() ||
      request_state_.Closed()) {
    // Don't emit any event after request cancel.
    return;
  }
  response_headers_ = response_headers;
  response_state_.SetFlag(ResponseStateFlags::kStarted);
  Emit("response");
}

void URLRequest::OnResponseData(
    scoped_refptr<const net::IOBufferWithSize> buffer) {
  if (request_state_.Canceled() ||
      request_state_.Closed() ||
      request_state_.Failed() ||
      response_state_.Failed()) {
    // In case we received an unexpected event from Chromium net,
    // don't emit any data event after request cancel/error/close.
    return;
  }
  if (!buffer || !buffer->data() || !buffer->size()) {
    return;
  }
  EmitResponseEvent(false, "data", buffer);
}

void URLRequest::OnResponseCompleted() {
  if (request_state_.Canceled() ||
    request_state_.Closed() ||
    request_state_.Failed() ||
    response_state_.Failed()) {
    // In case we received an unexpected event from Chromium net,
    // don't emit any data event after request cancel/error/close.
    return;
  }
  response_state_.SetFlag(ResponseStateFlags::kEnded);
  EmitResponseEvent(false, "end");
  Close();
}

void URLRequest::OnRequestError(const std::string& error) {
  request_state_.SetFlag(RequestStateFlags::kFailed);
  auto error_object = v8::Exception::Error(mate::StringToV8(isolate(), error));
  EmitRequestEvent(false, "error", error_object);
  Close();
}

void URLRequest::OnResponseError(const std::string& error) {
  response_state_.SetFlag(ResponseStateFlags::kFailed);
  auto error_object = v8::Exception::Error(mate::StringToV8(isolate(), error));
  EmitResponseEvent(false, "error", error_object);
  Close();
}


int URLRequest::StatusCode() const {
  if (response_headers_) {
    return response_headers_->response_code();
  }
  return -1;
}

std::string URLRequest::StatusMessage() const {
  std::string result;
  if (response_headers_) {
    result = response_headers_->GetStatusText();
  }
  return result;
}

scoped_refptr<const net::HttpResponseHeaders>
URLRequest::RawResponseHeaders() const {
  return response_headers_;
}

uint32_t URLRequest::ResponseHttpVersionMajor() const {
  if (response_headers_) {
     return response_headers_->GetHttpVersion().major_value();
  }
  return 0;
}

uint32_t URLRequest::ResponseHttpVersionMinor() const {
  if (response_headers_) {
    return response_headers_->GetHttpVersion().minor_value();
  }
  return 0;
}

void URLRequest::Close() {
  if (!request_state_.Closed()) {
    request_state_.SetFlag(RequestStateFlags::kClosed);
    if (response_state_.Started()) {
      // Emit a close event if we really have a response object.
      EmitResponseEvent(true, "close");
    }
    EmitRequestEvent(true, "close");
  }
  unpin();
  atom_request_ = nullptr;
}

void URLRequest::pin() {
  if (wrapper_.IsEmpty()) {
    wrapper_.Reset(isolate(), GetWrapper());
  }
}

void URLRequest::unpin() {
  wrapper_.Reset();
}

}  // namespace api

}  // namespace atom
