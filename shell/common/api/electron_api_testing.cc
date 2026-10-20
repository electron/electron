// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/power_monitor/power_monitor_source.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "content/browser/network_service_instance_impl.h"  // nogncheck
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_switches.h"
#include "gin/array_buffer.h"
#include "shell/browser/native_peer.h"
#include "shell/browser/native_window.h"
#include "shell/browser/webauthn/electron_authenticator_request_client_delegate.h"
#include "shell/browser/window_list.h"
#include "shell/common/callback_util.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/function_template.h"
#include "shell/common/gin_helper/function_template_cache.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_code_cache_test_helpers.h"  // nogncheck
#include "ui/accessibility/platform/ax_platform.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-wasm.h"
#include "v8/include/v8.h"

#if BUILDFLAG(IS_LINUX)
#include <glib.h>
#elif BUILDFLAG(IS_MAC)
#include <CoreFoundation/CoreFoundation.h>
#elif BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

namespace node::crypto {
NODE_EXTERN void AddBlockedSPKIForTesting(const uint8_t* sha256_hash);
NODE_EXTERN void ClearBlockedSPKIsForTesting();
}  // namespace node::crypto

namespace {

std::atomic<int> live_callback_holder_probes{0};

class CallbackHolderProbe {
 public:
  CallbackHolderProbe() { ++live_callback_holder_probes; }
  ~CallbackHolderProbe() { --live_callback_holder_probes; }

  int Run() const { return 42; }
};

v8::Local<v8::Function> CreateCallbackHolderProbeForTesting(
    v8::Isolate* isolate) {
  auto callback = base::BindRepeating(
      [](const std::shared_ptr<CallbackHolderProbe>& probe) {
        return probe->Run();
      },
      std::make_shared<CallbackHolderProbe>());
  using HolderT = gin_helper::CallbackHolder<int()>;
  auto* holder = cppgc::MakeGarbageCollected<HolderT>(
      isolate->GetCppHeap()->GetAllocationHandle(), std::move(callback),
      gin_helper::InvokerOptions{});
  return v8::Function::New(isolate->GetCurrentContext(),
                           &gin_helper::Dispatcher<int()>::DispatchToCallback,
                           holder->GetHandle(isolate))
      .ToLocalChecked();
}

v8::Local<v8::Function> GetCachedCallbackTemplateForTesting(
    v8::Isolate* isolate) {
  static const char kCacheKey = 0;
  auto context = isolate->GetCurrentContext();
  auto tmpl = gin_helper::GetCachedFunctionTemplate(isolate, &kCacheKey);
  if (tmpl.IsEmpty()) {
    tmpl = gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating([]() { return 42; }));
    gin_helper::SetCachedFunctionTemplate(isolate, &kCacheKey, tmpl);
  }
  return tmpl->GetFunction(context).ToLocalChecked();
}

int GetLiveCallbackHolderProbeCountForTesting() {
  return live_callback_holder_probes.load();
}

v8::Local<v8::Value> SerializeWasmModuleForTesting(
    v8::Isolate* isolate,
    gin_helper::ErrorThrower thrower,
    v8::Local<v8::Value> module) {
  if (!module->IsWasmModuleObject()) {
    thrower.ThrowTypeError("Expected a WebAssembly.Module");
    return {};
  }
  auto serialized =
      module.As<v8::WasmModuleObject>()->GetCompiledModule().Serialize();
  return node::Buffer::Copy(
             isolate, reinterpret_cast<const char*>(serialized.buffer.get()),
             serialized.size)
      .ToLocalChecked();
}

v8::Local<v8::Value> CompileWasmModuleWithCacheForTesting(
    v8::Isolate* isolate,
    const gin::ArrayBufferView& wire_bytes,
    const gin::ArrayBufferView& cached_bytes) {
  auto promise =
      std::make_unique<gin_helper::Promise<v8::Local<v8::Value>>>(isolate);
  auto handle = promise->GetHandle();
  using ModuleResult =
      std::variant<v8::Local<v8::WasmModuleObject>, v8::Local<v8::Value>>;
  auto resolve = electron::AdaptCallbackForRepeating(base::BindOnce(
      [](std::unique_ptr<gin_helper::Promise<v8::Local<v8::Value>>> promise,
         ModuleResult result) {
        if (auto* module =
                std::get_if<v8::Local<v8::WasmModuleObject>>(&result)) {
          v8::Local<v8::Value> value = *module;
          promise->Resolve(value);
        } else {
          promise->Reject(std::get<v8::Local<v8::Value>>(result));
        }
      },
      std::move(promise)));
  v8::WasmModuleCompilation compilation;
  compilation.SetHasCompiledModuleBytes();
  compilation.OnBytesReceived(wire_bytes.span().data(),
                              wire_bytes.span().size());
  bool accepted = false;
  compilation.Finish(
      isolate,
      [&accepted,
       &cached_bytes](v8::WasmStreaming::ModuleCachingInterface& cache) {
        accepted = cache.SetCachedCompiledModuleBytes(
            std::span(cached_bytes.span().data(), cached_bytes.span().size()));
      },
      [resolve = std::move(resolve)](ModuleResult result) {
        resolve.Run(std::move(result));
      });
  auto result = gin_helper::Dictionary::CreateEmpty(isolate);
  result.Set("accepted", accepted);
  result.Set("module", handle);
  return result.GetHandle();
}

class CallbackTestingHelper final {
 public:
  void HoldRepeatingCallback(const base::RepeatingClosure& callback) {
    repeating_callback_ = callback;
  }

  bool CopyHeldRepeatingCallback() {
    if (!repeating_callback_)
      return false;

    repeating_callback_copy_ = *repeating_callback_;
    return true;
  }

  bool InvokeHeldRepeatingCallback(v8::Isolate* isolate) {
    if (!repeating_callback_)
      return false;

    return InvokeRepeatingCallback(isolate, *repeating_callback_);
  }

  bool InvokeCopiedRepeatingCallback(v8::Isolate* isolate) {
    if (!repeating_callback_copy_)
      return false;

    return InvokeRepeatingCallback(isolate, *repeating_callback_copy_);
  }

  void HoldOnceCallback(base::OnceClosure callback) {
    once_callback_ = std::move(callback);
  }

  bool InvokeHeldOnceCallback(v8::Isolate* isolate) {
    if (!once_callback_)
      return false;

    base::OnceClosure callback = std::move(*once_callback_);
    once_callback_.reset();
    return InvokeOnceCallback(isolate, std::move(callback));
  }

  void ClearPrimaryHeldRepeatingCallback() { repeating_callback_.reset(); }

  int GetHeldRepeatingCallbackCount() const {
    return (repeating_callback_ ? 1 : 0) + (repeating_callback_copy_ ? 1 : 0);
  }

  void ClearAllHeldCallbacks() {
    repeating_callback_.reset();
    repeating_callback_copy_.reset();
    once_callback_.reset();
  }

 private:
  bool InvokeRepeatingCallback(v8::Isolate* isolate,
                               const base::RepeatingClosure& callback) {
    v8::TryCatch try_catch(isolate);
    callback.Run();
    if (try_catch.HasCaught()) {
      try_catch.Reset();
      return false;
    }

    return true;
  }

  bool InvokeOnceCallback(v8::Isolate* isolate, base::OnceClosure callback) {
    v8::TryCatch try_catch(isolate);
    std::move(callback).Run();
    if (try_catch.HasCaught()) {
      try_catch.Reset();
      return false;
    }

    return true;
  }

  std::optional<base::RepeatingClosure> repeating_callback_;
  std::optional<base::RepeatingClosure> repeating_callback_copy_;
  std::optional<base::OnceClosure> once_callback_;
};

CallbackTestingHelper& GetCallbackTestingHelper() {
  static base::NoDestructor<CallbackTestingHelper> helper;
  return *helper;
}

void Log(int severity, std::string text) {
  switch (severity) {
    case logging::LOGGING_VERBOSE:
      VLOG(1) << text;
      break;
    case logging::LOGGING_INFO:
      LOG(INFO) << text;
      break;
    case logging::LOGGING_WARNING:
      LOG(WARNING) << text;
      break;
    case logging::LOGGING_ERROR:
      LOG(ERROR) << text;
      break;
    case logging::LOGGING_FATAL:
      LOG(FATAL) << text;
      // break not needed here because LOG(FATAL) is [[noreturn]]
    default:
      LOG(ERROR) << "Unrecognized severity: " << severity;
      break;
  }
}

std::string GetLoggingDestination() {
  const auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->GetSwitchValueASCII(switches::kEnableLogging);
}

bool IsPlatformCaretBrowsingEnabled() {
  return ui::AXPlatform::GetInstance().IsCaretBrowsingEnabled();
}

v8::Local<v8::Promise> SimulateNetworkServiceCrash(v8::Isolate* isolate) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  auto subscription = content::RegisterNetworkServiceProcessGoneHandler(
      electron::AdaptCallbackForRepeating(
          base::BindOnce([](gin_helper::Promise<void> promise,
                            bool crashed) { promise.Resolve(); },
                         std::move(promise))));
  content::RestartNetworkService();
  return handle;
}

void AddNodeBlockedSPKIForTesting(gin_helper::ErrorThrower thrower,
                                  const std::string& hex) {
  std::vector<uint8_t> hash;
  if (!base::HexStringToBytes(hex, &hash) || hash.size() != 32) {
    thrower.ThrowTypeError("expected a 64 character hex SHA-256");
    return;
  }
  node::crypto::AddBlockedSPKIForTesting(hash.data());
}

void ClearNodeBlockedSPKIsForTesting() {
  node::crypto::ClearBlockedSPKIsForTesting();
}

void HoldRepeatingCallbackForTesting(const base::RepeatingClosure& callback) {
  GetCallbackTestingHelper().HoldRepeatingCallback(callback);
}

bool CopyHeldRepeatingCallbackForTesting() {
  return GetCallbackTestingHelper().CopyHeldRepeatingCallback();
}

bool InvokeHeldRepeatingCallbackForTesting(gin::Arguments* args) {
  return GetCallbackTestingHelper().InvokeHeldRepeatingCallback(
      args->isolate());
}

bool InvokeCopiedRepeatingCallbackForTesting(gin::Arguments* args) {
  return GetCallbackTestingHelper().InvokeCopiedRepeatingCallback(
      args->isolate());
}

void HoldOnceCallbackForTesting(base::OnceClosure callback) {
  GetCallbackTestingHelper().HoldOnceCallback(std::move(callback));
}

bool InvokeHeldOnceCallbackForTesting(gin::Arguments* args) {
  return GetCallbackTestingHelper().InvokeHeldOnceCallback(args->isolate());
}

void ClearPrimaryHeldRepeatingCallbackForTesting() {
  GetCallbackTestingHelper().ClearPrimaryHeldRepeatingCallback();
}

int GetHeldRepeatingCallbackCountForTesting() {
  return GetCallbackTestingHelper().GetHeldRepeatingCallbackCount();
}

void ClearHeldCallbacksForTesting() {
  GetCallbackTestingHelper().ClearAllHeldCallbacks();
}

// Intentionally allows exit-time destructor so that PromiseBase's destructor
// runs after CppHeap teardown.
std::optional<gin_helper::Promise<void>>& GetHeldPromise() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
  static std::optional<gin_helper::Promise<void>> held_promise;
#pragma clang diagnostic pop
  return held_promise;
}

// Fires every window's pending debounced window-state save now. Bounds only
// reach PrefService when the 200ms timer started by
// NativeWindow::DebouncedSaveWindowState runs, which a test would otherwise
// have to sleep through before committing the write below.
void FlushPendingWindowStateSaves() {
  for (auto* window : electron::WindowList::GetWindows())
    window->FlushPendingWindowStateSaveForTesting();
}

// Writes any pending local state (window state persistence, per-host zoom
// levels, ...) to disk now. PrefService otherwise batches writes on a 10s
// timer, which is what a test polling the prefs file would wait on.
v8::Local<v8::Promise> CommitPendingLocalStateWrites(v8::Isolate* isolate) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  PrefService* local_state =
      g_browser_process ? g_browser_process->local_state() : nullptr;
  if (!local_state) {
    promise.RejectWithErrorMessage("No local state in this process");
    return handle;
  }
  local_state->CommitPendingWrite(base::BindOnce(
      [](gin_helper::Promise<void> promise) { promise.Resolve(); },
      std::move(promise)));
  return handle;
}

void HoldPromiseForTesting(gin::Arguments* args) {
  GetHeldPromise().emplace(args->isolate());
}

void ClearHeldPromiseForTesting() {
  GetHeldPromise().reset();
}

// Settles a promise from a native event source callback, the way an X11 reply
// or an OS event handler would: on the UI thread, but not from inside a task.
// Once it has, |after_settle| is posted as an ordinary task, so a test can
// check that the promise's continuations ran before that task did.
struct SettleOutsideTask {
  gin_helper::Promise<void> promise;
  base::OnceClosure after_settle;

  static void Run(std::unique_ptr<SettleOutsideTask> self) {
    self->promise.Resolve();
    content::GetUIThreadTaskRunner({})->PostTask(FROM_HERE,
                                                 std::move(self->after_settle));
  }

#if BUILDFLAG(IS_LINUX)
  static gboolean OnIdle(gpointer data) {
    Run(base::WrapUnique(static_cast<SettleOutsideTask*>(data)));
    return G_SOURCE_REMOVE;
  }
#elif BUILDFLAG(IS_MAC)
  static void OnTimer(CFRunLoopTimerRef timer, void* info) {
    Run(base::WrapUnique(static_cast<SettleOutsideTask*>(info)));
    CFRunLoopTimerInvalidate(timer);
    CFRelease(timer);
  }
#elif BUILDFLAG(IS_WIN)
  static SettleOutsideTask*& Pending() {
    static SettleOutsideTask* pending = nullptr;
    return pending;
  }
  static void CALLBACK OnTimer(HWND, UINT, UINT_PTR id, DWORD) {
    ::KillTimer(nullptr, id);
    if (auto* self = std::exchange(Pending(), nullptr))
      Run(base::WrapUnique(self));
  }
#endif
};

v8::Local<v8::Promise> SettlePromiseOutsideTask(
    v8::Isolate* isolate,
    base::OnceClosure after_settle) {
  auto state = std::make_unique<SettleOutsideTask>(SettleOutsideTask{
      gin_helper::Promise<void>(isolate), std::move(after_settle)});
  v8::Local<v8::Promise> handle = state->promise.GetHandle();
#if BUILDFLAG(IS_LINUX)
  g_idle_add(&SettleOutsideTask::OnIdle, state.release());
#elif BUILDFLAG(IS_MAC)
  CFRunLoopTimerContext context = {0, state.release(), nullptr, nullptr,
                                   nullptr};
  CFRunLoopTimerRef timer =
      CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), 0,
                           0, 0, &SettleOutsideTask::OnTimer, &context);
  CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
#elif BUILDFLAG(IS_WIN)
  CHECK(!SettleOutsideTask::Pending());
  SettleOutsideTask::Pending() = state.release();
  ::SetTimer(nullptr, 0, USER_TIMER_MINIMUM, &SettleOutsideTask::OnTimer);
#endif
  return handle;
}

// Runs |before_collection|, a full garbage collection, and then
// |before_finalization| while the collection's dead cppgc objects are still
// waiting to be swept, as a natural collection leaves them until a later task
// sweeps. Everything runs in one fresh non-nestable task. That task starts from
// the message loop, so no native frame below it holds a cppgc pointer, which
// makes kNoHeapPointers accurate and lets the collection skip the stack. It
// also means no sweeping task runs in between. Sweeping finishes before the
// promise resolves.
//
// It watches |target|, a wrapper object that |before_collection| is expected to
// make unreachable:
//  - |before_finalization| is told whether the collection found it dead.
//  - The promise rejects if the target's wrapper was finalized before or during
//    |before_finalization|, which would leave the callback running too late.
//    Its native peer is queued for release exactly when it is finalized.
//  - The promise resolves with whether the target was dead and finalized by the
//    sweep that followed.
// The target is read when this is called and only watched weakly afterwards, so
// it stays collectable. Requires --js-flags=--expose-gc.
void CollectBeforeFinalization(
    v8::Isolate* isolate,
    cppgc::WeakPersistent<v8::Object::Wrappable> watched,
    uintptr_t identity,
    base::OnceClosure before_collection,
    base::OnceCallback<void(bool)> before_finalization,
    gin_helper::Promise<bool> promise) {
  std::move(before_collection).Run();

  v8::CppHeap* cpp_heap = isolate->GetCppHeap();
  cpp_heap->SetForceIncrementalSweepingForTesting(true);
  isolate->RequestGarbageCollectionForTesting(
      v8::Isolate::GarbageCollectionType::kFullGarbageCollection,
      v8::StackState::kNoHeapPointers);

  const bool dead = !watched;
  // Peers are only added to the queue within this task. They are released by a
  // posted non-nestable task. So a check after the callback also covers the
  // time before it.
  if (!electron::NativePeerBase::IsQueuedForTesting(identity))
    std::move(before_finalization).Run(dead);
  const bool finalized_early =
      electron::NativePeerBase::IsQueuedForTesting(identity);

  cpp_heap->FinishSweepingForTesting();
  cpp_heap->SetForceIncrementalSweepingForTesting(false);

  if (finalized_early) {
    promise.RejectWithErrorMessage(
        "The target was finalized before or during the callback");
    return;
  }
  promise.Resolve(dead &&
                  electron::NativePeerBase::IsQueuedForTesting(identity));
}

v8::Local<v8::Promise> CollectGarbageBeforeFinalizationForTesting(
    v8::Isolate* isolate,
    v8::Local<v8::Object> target,
    base::OnceClosure before_collection,
    base::OnceCallback<void(bool)> before_finalization) {
  gin_helper::Promise<bool> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  v8::Object::Wrappable* wrappable = nullptr;
  if (target->IsApiWrapper()) {
    wrappable = v8::Object::Unwrap<v8::Object::Wrappable>(
        isolate, target, v8::kObjectWrappableTagRange);
  }
  if (!wrappable) {
    promise.RejectWithErrorMessage("The target is not a wrapper object");
    return handle;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE,
      base::BindOnce(&CollectBeforeFinalization, base::Unretained(isolate),
                     cppgc::WeakPersistent<v8::Object::Wrappable>(wrappable),
                     reinterpret_cast<uintptr_t>(wrappable),
                     std::move(before_collection),
                     std::move(before_finalization), std::move(promise)));
  return handle;
}

// Reaches the protected PowerMonitorSource::ProcessPowerEvent().
struct PowerEventInjector : base::PowerMonitorSource {
  static void Inject(PowerEvent event) { ProcessPowerEvent(event); }
};

void SimulatePowerEvent(gin_helper::ErrorThrower thrower,
                        const std::string& event) {
  if (event == "suspend")
    PowerEventInjector::Inject(base::PowerMonitorSource::SUSPEND_EVENT);
  else if (event == "resume")
    PowerEventInjector::Inject(base::PowerMonitorSource::RESUME_EVENT);
  else
    thrower.ThrowTypeError("unknown power event");
}

void SimulateWebAuthnUvLockedPinSecurityKey(bool enabled) {
  electron::ElectronAuthenticatorRequestClientDelegate::
      SetSimulateUvLockedPinSecurityKeyForTesting(enabled);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod<&CreateCallbackHolderProbeForTesting>(
      "createCallbackHolderProbeForTesting");
  dict.SetMethod<&GetCachedCallbackTemplateForTesting>(
      "getCachedCallbackTemplateForTesting");
  dict.SetMethod<&GetLiveCallbackHolderProbeCountForTesting>(
      "getLiveCallbackHolderProbeCountForTesting");
  dict.SetMethod<&electron::testing::ComputeV8VersionHash>(
      "computeV8VersionHashForTesting");
  dict.SetMethod<&SerializeWasmModuleForTesting>(
      "serializeWasmModuleForTesting");
  dict.SetMethod<&CompileWasmModuleWithCacheForTesting>(
      "compileWasmModuleWithCacheForTesting");
  dict.SetMethod<&Log>("log");
  dict.SetMethod<&GetLoggingDestination>("getLoggingDestination");
  dict.SetMethod<&IsPlatformCaretBrowsingEnabled>(
      "isPlatformCaretBrowsingEnabled");
  dict.SetMethod<&SimulateNetworkServiceCrash>("simulateNetworkServiceCrash");
  dict.SetMethod<&AddNodeBlockedSPKIForTesting>("addNodeBlockedSPKI");
  dict.SetMethod<&ClearNodeBlockedSPKIsForTesting>("clearNodeBlockedSPKIs");
  dict.SetMethod<&SimulatePowerEvent>("simulatePowerEvent");
  dict.SetMethod<&SimulateWebAuthnUvLockedPinSecurityKey>(
      "simulateWebAuthnUvLockedPinSecurityKey");
  dict.SetMethod<&HoldRepeatingCallbackForTesting>(
      "holdRepeatingCallbackForTesting");
  dict.SetMethod<&CopyHeldRepeatingCallbackForTesting>(
      "copyHeldRepeatingCallbackForTesting");
  dict.SetMethod<&InvokeHeldRepeatingCallbackForTesting>(
      "invokeHeldRepeatingCallbackForTesting");
  dict.SetMethod<&InvokeCopiedRepeatingCallbackForTesting>(
      "invokeCopiedRepeatingCallbackForTesting");
  dict.SetMethod<&ClearPrimaryHeldRepeatingCallbackForTesting>(
      "clearPrimaryHeldRepeatingCallbackForTesting");
  dict.SetMethod<&GetHeldRepeatingCallbackCountForTesting>(
      "getHeldRepeatingCallbackCountForTesting");
  dict.SetMethod<&HoldOnceCallbackForTesting>("holdOnceCallbackForTesting");
  dict.SetMethod<&InvokeHeldOnceCallbackForTesting>(
      "invokeHeldOnceCallbackForTesting");
  dict.SetMethod<&ClearHeldCallbacksForTesting>("clearHeldCallbacksForTesting");
  dict.SetMethod<&HoldPromiseForTesting>("holdPromiseForTesting");
  dict.SetMethod<&SettlePromiseOutsideTask>("settlePromiseOutsideTask");
  dict.SetMethod<&CollectGarbageBeforeFinalizationForTesting>(
      "collectGarbageBeforeFinalizationForTesting");
  dict.SetMethod<&FlushPendingWindowStateSaves>("flushPendingWindowStateSaves");
  dict.SetMethod<&CommitPendingLocalStateWrites>(
      "commitPendingLocalStateWrites");
  dict.SetMethod<&ClearHeldPromiseForTesting>("clearHeldPromiseForTesting");
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_testing, Initialize)
