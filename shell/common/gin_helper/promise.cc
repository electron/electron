// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <string_view>

#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/process_util.h"
#include "v8/include/v8-context.h"

namespace gin_helper {

PromiseBase::SettleScope::SettleScope(const PromiseBase& base)
    : handle_scope_{base.isolate()},
      context_{base.GetContext()},
      microtasks_scope_{base.isolate(), context_->GetMicrotaskQueue(), false,
                        v8::MicrotasksScope::kRunMicrotasks},
      context_scope_{context_} {}

PromiseBase::SettleScope::~SettleScope() = default;

PromiseBase::PromiseBase(v8::Isolate* isolate)
    : PromiseBase(isolate,
                  v8::Promise::Resolver::New(isolate->GetCurrentContext())
                      .ToLocalChecked()) {}

PromiseBase::PromiseBase(v8::Isolate* isolate,
                         v8::Local<v8::Promise::Resolver> handle)
    : isolate_(isolate),
      context_(isolate, isolate->GetCurrentContext()),
      resolver_(isolate, handle) {}

PromiseBase::PromiseBase() : isolate_(nullptr) {}

PromiseBase::PromiseBase(PromiseBase&&) = default;

PromiseBase::~PromiseBase() = default;

PromiseBase& PromiseBase::operator=(PromiseBase&&) = default;

// static
scoped_refptr<base::TaskRunner> PromiseBase::GetTaskRunner() {
  if (electron::IsBrowserProcess() &&
      !content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    return content::GetUIThreadTaskRunner({});
  }
  return {};
}

v8::Maybe<bool> PromiseBase::Reject(v8::Local<v8::Value> except) {
  SettleScope settle_scope{*this};
  return GetInner()->Reject(settle_scope.context_, except);
}

v8::Maybe<bool> PromiseBase::Reject() {
  SettleScope settle_scope{*this};
  return GetInner()->Reject(settle_scope.context_, v8::Undefined(isolate()));
}

v8::Maybe<bool> PromiseBase::RejectWithErrorMessage(std::string_view errmsg) {
  SettleScope settle_scope{*this};
  return GetInner()->Reject(
      settle_scope.context_,
      v8::Exception::Error(gin::StringToV8(isolate(), errmsg)));
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
void PromiseBase::RejectPromise(PromiseBase&& promise,
                                std::string_view errmsg) {
  if (auto task_runner = GetTaskRunner()) {
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(
                       // Note that this callback can not take std::string_view,
                       // as StringPiece only references string internally and
                       // will blow when a temporary string is passed.
                       [](PromiseBase&& promise, std::string str) {
                         promise.RejectWithErrorMessage(str);
                       },
                       std::move(promise), std::string{errmsg}));
  } else {
    promise.RejectWithErrorMessage(errmsg);
  }
}

// static
void Promise<void>::ResolvePromise(Promise<void> promise) {
  if (auto task_runner = GetTaskRunner()) {
    task_runner->PostTask(
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
  SettleScope settle_scope{*this};
  return GetInner()->Resolve(settle_scope.context_, v8::Undefined(isolate()));
}

}  // namespace gin_helper
