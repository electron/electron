// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/promise_util.h"

namespace electron {

namespace util {

PromiseBase::PromiseBase(v8::Isolate* isolate)
    : PromiseBase(isolate,
                  v8::Promise::Resolver::New(isolate->GetCurrentContext())
                      .ToLocalChecked()) {}

PromiseBase::PromiseBase(v8::Isolate* isolate,
                         v8::Local<v8::Promise::Resolver> handle)
    : isolate_(isolate),
      context_(isolate, isolate->GetCurrentContext()),
      resolver_(isolate, handle) {}

PromiseBase::PromiseBase(PromiseBase&&) = default;

PromiseBase::~PromiseBase() = default;

PromiseBase& PromiseBase::operator=(PromiseBase&&) = default;

v8::Maybe<bool> PromiseBase::Reject() {
  v8::HandleScope handle_scope(isolate());
  v8::MicrotasksScope script_scope(isolate(),
                                   v8::MicrotasksScope::kRunMicrotasks);
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(isolate(), GetContext()));

  return GetInner()->Reject(GetContext(), v8::Undefined(isolate()));
}

v8::Maybe<bool> PromiseBase::Reject(v8::Local<v8::Value> except) {
  v8::HandleScope handle_scope(isolate());
  v8::MicrotasksScope script_scope(isolate(),
                                   v8::MicrotasksScope::kRunMicrotasks);
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(isolate(), GetContext()));

  return GetInner()->Reject(GetContext(), except);
}

v8::Maybe<bool> PromiseBase::RejectWithErrorMessage(base::StringPiece message) {
  v8::HandleScope handle_scope(isolate());
  v8::MicrotasksScope script_scope(isolate(),
                                   v8::MicrotasksScope::kRunMicrotasks);
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(isolate(), GetContext()));

  v8::Local<v8::Value> error =
      v8::Exception::Error(gin::StringToV8(isolate(), message));
  return GetInner()->Reject(GetContext(), (error));
}

v8::Local<v8::Context> PromiseBase::GetContext() const {
  return v8::Local<v8::Context>::New(isolate_, context_);
}

v8::Local<v8::Promise> PromiseBase::GetHandle() const {
  return GetInner()->GetPromise();
}

v8::Local<v8::Promise::Resolver> PromiseBase::GetInner() const {
  return resolver_.Get(isolate());
}

}  // namespace util

}  // namespace electron
