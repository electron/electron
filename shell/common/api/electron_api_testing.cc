// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/dcheck_is_on.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "base/power_monitor/power_monitor_source.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "content/browser/network_service_instance_impl.h"  // nogncheck
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_switches.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/callback_util.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "ui/accessibility/platform/ax_platform.h"
#include "v8/include/v8.h"

#if BUILDFLAG(IS_LINUX)
#include <glib.h>
#elif BUILDFLAG(IS_MAC)
#include <CoreFoundation/CoreFoundation.h>
#elif BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

#if DCHECK_IS_ON()
namespace {

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

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod<&Log>("log");
  dict.SetMethod<&GetLoggingDestination>("getLoggingDestination");
  dict.SetMethod<&IsPlatformCaretBrowsingEnabled>(
      "isPlatformCaretBrowsingEnabled");
  dict.SetMethod<&SimulateNetworkServiceCrash>("simulateNetworkServiceCrash");
  dict.SetMethod<&SimulatePowerEvent>("simulatePowerEvent");
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
  dict.SetMethod<&FlushPendingWindowStateSaves>("flushPendingWindowStateSaves");
  dict.SetMethod<&CommitPendingLocalStateWrites>(
      "commitPendingLocalStateWrites");
  dict.SetMethod<&ClearHeldPromiseForTesting>("clearHeldPromiseForTesting");
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_testing, Initialize)
#endif
