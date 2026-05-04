// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <string_view>

#include "base/debug/stack_trace.h"
#include "base/threading/platform_thread.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/process_util.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/heap-statistics.h"
#include "v8/include/cppgc/name-provider.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-statistics.h"
#include "v8/include/v8-traced-handle.h"

namespace gin_helper {

namespace {

void EmitIsolateState(perfetto::EventContext& ctx, v8::Isolate* isolate) {
  ctx.AddDebugAnnotation("thread_id", base::PlatformThread::CurrentId().raw());
  ctx.AddDebugAnnotation("is_browser_process", electron::IsBrowserProcess());
  if (!isolate) {
    ctx.AddDebugAnnotation("isolate", "null");
    return;
  }
  ctx.AddDebugAnnotation("isolate", reinterpret_cast<uintptr_t>(isolate));
  ctx.AddDebugAnnotation("isolate.in_context", isolate->InContext());
  ctx.AddDebugAnnotation("isolate.has_pending_exception",
                         isolate->HasPendingException());

  v8::HeapStatistics hs;
  isolate->GetHeapStatistics(&hs);
  ctx.AddDebugAnnotation("v8.used_heap_size", hs.used_heap_size());
  ctx.AddDebugAnnotation("v8.total_heap_size", hs.total_heap_size());
  ctx.AddDebugAnnotation("v8.number_of_native_contexts",
                         hs.number_of_native_contexts());
  ctx.AddDebugAnnotation("v8.total_global_handles_size",
                         hs.total_global_handles_size());

  if (auto* cpp_heap = isolate->GetCppHeap()) {
    auto cpp_stats = cpp_heap->CollectStatistics(cppgc::HeapStatistics::kBrief);
    ctx.AddDebugAnnotation("cppgc.committed_size_bytes",
                           cpp_stats.committed_size_bytes);
    ctx.AddDebugAnnotation("cppgc.resident_size_bytes",
                           cpp_stats.resident_size_bytes);
    ctx.AddDebugAnnotation("cppgc.used_size_bytes", cpp_stats.used_size_bytes);
  }
}

void EmitStackTrace(perfetto::EventContext& ctx, const char* name) {
  ctx.AddDebugAnnotation(name, base::debug::StackTrace().ToString());
}

}  // namespace

class PromiseHandle final : public cppgc::GarbageCollected<PromiseHandle>,
                            public cppgc::NameProvider {
 public:
  PromiseHandle(v8::Isolate* isolate,
                v8::Local<v8::Context> context,
                v8::Local<v8::Promise::Resolver> resolver)
      : context_(isolate, context), resolver_(isolate, resolver) {
    TRACE_EVENT("electron", "PromiseHandle::ctor",
                perfetto::Flow::FromPointer(this, "PromiseHandle"),
                [&](perfetto::EventContext ctx) {
                  ctx.AddDebugAnnotation("this",
                                         reinterpret_cast<uintptr_t>(this));
                  ctx.AddDebugAnnotation("context_empty", context.IsEmpty());
                  ctx.AddDebugAnnotation("resolver_empty", resolver.IsEmpty());
                  EmitIsolateState(ctx, isolate);
                  EmitStackTrace(ctx, "stack");
                });
  }

  void Trace(cppgc::Visitor* visitor) const {
    visitor->Trace(context_);
    visitor->Trace(resolver_);
  }

  const char* GetHumanReadableName() const final {
    return "Electron / PromiseHandle";
  }

  [[nodiscard]] bool IsAlive() const { return !resolver_.IsEmpty(); }

  v8::Local<v8::Context> GetContext(v8::Isolate* isolate) const {
    return context_.Get(isolate);
  }

  v8::Local<v8::Promise::Resolver> GetResolver(v8::Isolate* isolate) const {
    return resolver_.Get(isolate);
  }

 private:
  v8::TracedReference<v8::Context> context_;
  v8::TracedReference<v8::Promise::Resolver> resolver_;
};

PromiseBase::SettleScope::SettleScope(const PromiseBase& base)
    : isolate_{base.isolate()},
      handle_scope_{isolate_},
      context_{base.GetContext()},
      microtasks_scope_(context_, v8::MicrotasksScope::kRunMicrotasks),
      context_scope_{context_} {
  TRACE_EVENT_BEGIN(
      "electron", "PromiseBase::SettleScope",
      perfetto::Flow::FromPointer(&base, "PromiseBase"),
      [&](perfetto::EventContext ctx) {
        ctx.AddDebugAnnotation("promise", reinterpret_cast<uintptr_t>(&base));
        ctx.AddDebugAnnotation("context_empty", context_.IsEmpty());
        EmitIsolateState(ctx, isolate_);
      });
}

PromiseBase::SettleScope::~SettleScope() {
  if (electron::IsBrowserProcess()) {
    TRACE_EVENT(
        "electron", "PromiseBase::SettleScope::PerformCheckpoint",
        [&](perfetto::EventContext ctx) { EmitIsolateState(ctx, isolate_); });
    context_->GetMicrotaskQueue()->PerformCheckpoint(isolate_);
  }
  TRACE_EVENT_END("electron");
}

PromiseBase::PromiseBase(v8::Isolate* isolate)
    : PromiseBase(isolate,
                  v8::Promise::Resolver::New(isolate->GetCurrentContext())
                      .ToLocalChecked()) {}

PromiseBase::PromiseBase(v8::Isolate* isolate,
                         v8::Local<v8::Promise::Resolver> handle)
    : isolate_(isolate),
      handle_(cppgc::MakeGarbageCollected<PromiseHandle>(
          isolate->GetCppHeap()->GetAllocationHandle(),
          isolate,
          isolate->GetCurrentContext(),
          handle)) {
  TRACE_EVENT("electron", "PromiseBase::ctor",
              perfetto::Flow::FromPointer(this, "PromiseBase"),
              [&](perfetto::EventContext ctx) {
                ctx.AddDebugAnnotation("this",
                                       reinterpret_cast<uintptr_t>(this));
                ctx.AddDebugAnnotation(
                    "handle", reinterpret_cast<uintptr_t>(handle_.Get()));
                EmitIsolateState(ctx, isolate);
                EmitStackTrace(ctx, "stack");
              });
}

PromiseBase::PromiseBase() : isolate_(nullptr) {}

PromiseBase::PromiseBase(PromiseBase&&) = default;

PromiseBase::~PromiseBase() = default;

PromiseBase& PromiseBase::operator=(PromiseBase&&) = default;

bool PromiseBase::IsAlive() const {
  return handle_ && handle_->IsAlive();
}

// static
scoped_refptr<base::TaskRunner> PromiseBase::GetTaskRunner() {
  if (electron::IsBrowserProcess() &&
      !content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    return content::GetUIThreadTaskRunner({});
  }
  return {};
}

v8::Maybe<bool> PromiseBase::Reject(v8::Local<v8::Value> except) {
  if (!IsAlive())
    return v8::Nothing<bool>();
  SettleScope settle_scope{*this};
  return GetInner()->Reject(settle_scope.context_, except);
}

v8::Maybe<bool> PromiseBase::Reject() {
  if (!IsAlive())
    return v8::Nothing<bool>();
  SettleScope settle_scope{*this};
  return GetInner()->Reject(settle_scope.context_, v8::Undefined(isolate()));
}

v8::Maybe<bool> PromiseBase::RejectWithErrorMessage(std::string_view errmsg) {
  if (!IsAlive())
    return v8::Nothing<bool>();
  SettleScope settle_scope{*this};
  return GetInner()->Reject(
      settle_scope.context_,
      v8::Exception::Error(gin::StringToV8(isolate(), errmsg)));
}

v8::Maybe<bool> PromiseBase::Resolve() {
  TRACE_EVENT(
      "electron", "PromiseBase::Resolve",
      perfetto::Flow::FromPointer(this, "PromiseBase"),
      [&](perfetto::EventContext ctx) {
        ctx.AddDebugAnnotation("this", reinterpret_cast<uintptr_t>(this));
        ctx.AddDebugAnnotation("is_alive", IsAlive());
        ctx.AddDebugAnnotation("handle_present", static_cast<bool>(handle_));
        EmitIsolateState(ctx, isolate());
        EmitStackTrace(ctx, "stack");
      });
  if (!IsAlive())
    return v8::Nothing<bool>();
  return ResolveWith(v8::Undefined(isolate()));
}

v8::Local<v8::Context> PromiseBase::GetContext() const {
  return IsAlive() ? handle_->GetContext(isolate_) : v8::Local<v8::Context>();
}

v8::Local<v8::Promise> PromiseBase::GetHandle() const {
  auto inner = GetInner();
  return inner.IsEmpty() ? v8::Local<v8::Promise>() : inner->GetPromise();
}

v8::Local<v8::Promise::Resolver> PromiseBase::GetInner() const {
  return IsAlive() ? handle_->GetResolver(isolate_)
                   : v8::Local<v8::Promise::Resolver>();
}

v8::Maybe<bool> PromiseBase::ResolveWith(v8::Local<v8::Value> value) {
  TRACE_EVENT("electron", "PromiseBase::ResolveWith",
              perfetto::Flow::FromPointer(this, "PromiseBase"),
              [&](perfetto::EventContext ctx) {
                ctx.AddDebugAnnotation("this",
                                       reinterpret_cast<uintptr_t>(this));
                EmitIsolateState(ctx, isolate());
              });
  SettleScope settle_scope{*this};
  return GetInner()->Resolve(settle_scope.context_, value);
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

}  // namespace gin_helper
