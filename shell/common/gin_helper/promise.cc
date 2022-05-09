// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/promise.h"

namespace gin_helper {

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
  gin_helper::MicrotasksScope microtasks_scope(isolate());
  v8::Context::Scope context_scope(GetContext());

  return GetInner()->Reject(GetContext(), v8::Undefined(isolate()));
}

v8::Maybe<bool> PromiseBase::Reject(v8::Local<v8::Value> except) {
  v8::HandleScope handle_scope(isolate());
  gin_helper::MicrotasksScope microtasks_scope(isolate());
  v8::Context::Scope context_scope(GetContext());

  return GetInner()->Reject(GetContext(), except);
}

v8::Maybe<bool> PromiseBase::RejectWithErrorMessage(base::StringPiece message) {
  v8::HandleScope handle_scope(isolate());
  gin_helper::MicrotasksScope microtasks_scope(isolate());
  v8::Context::Scope context_scope(GetContext());

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

// static
void Promise<void>::ResolvePromise(Promise<void> promise) {
  if (gin_helper::Locker::IsBrowserProcess() &&
      !content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce([](Promise<void> promise) { promise.Resolve(); },
                       std::move(promise)));
  } else {
    promise.Resolve();
  }
}

// static
v8::Local<v8::Promise> Promise<void>::ResolvedPromise(v8::Isolate* isolate) {
  Promise<void> resolved(isolate);
  resolved.Resolve();
  return resolved.GetHandle();
}

v8::Maybe<bool> Promise<void>::Resolve() {
  v8::HandleScope handle_scope(isolate());
  gin_helper::MicrotasksScope microtasks_scope(isolate());
  v8::Context::Scope context_scope(GetContext());

  return GetInner()->Resolve(GetContext(), v8::Undefined(isolate()));
}

}  // namespace gin_helper
