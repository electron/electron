// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>
#include <string>

#include "base/command_line.h"
#include "base/dcheck_is_on.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/power_monitor/power_monitor_source.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "content/browser/network_service_instance_impl.h"  // nogncheck
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_switches.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "shell/browser/testing/fake_device_managers.h"
#include "shell/browser/window_list.h"
#include "shell/common/callback_util.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "ui/accessibility/platform/ax_platform.h"
#include "v8/include/v8.h"

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

// --- fake device managers -------------------------------------------------

electron::FakeDeviceManagers* FakeDevicesFor(gin::Arguments* args,
                                             v8::Local<v8::Value> session) {
  electron::api::Session* api_session = nullptr;
  if (!gin::ConvertFromV8(args->isolate(), session, &api_session) ||
      !api_session) {
    args->ThrowTypeError("session required");
    return nullptr;
  }
  return electron::FakeDeviceManagers::GetOrCreate(
      api_session->browser_context());
}

// Replaces the HID, USB and serial device managers of |session| with in-process
// fakes so specs can add and remove devices. Idempotent.
void UseFakeDeviceManagers(gin::Arguments* args, v8::Local<v8::Value> session) {
  FakeDevicesFor(args, session);
}

std::string AddFakeHidDevice(gin::Arguments* args,
                             v8::Local<v8::Value> session,
                             const gin_helper::Dictionary& opts) {
  auto* fakes = FakeDevicesFor(args, session);
  if (!fakes)
    return {};
  int vendor_id = 0, product_id = 0;
  std::string name = "Fake HID", serial;
  opts.Get("vendorId", &vendor_id);
  opts.Get("productId", &product_id);
  opts.Get("name", &name);
  opts.Get("serialNumber", &serial);
  return fakes->hid().AddDevice(vendor_id, product_id, name, serial);
}

void RemoveFakeHidDevice(gin::Arguments* args,
                         v8::Local<v8::Value> session,
                         const std::string& guid) {
  if (auto* fakes = FakeDevicesFor(args, session))
    fakes->hid().RemoveDevice(guid);
}

std::string AddFakeUsbDevice(gin::Arguments* args,
                             v8::Local<v8::Value> session,
                             const gin_helper::Dictionary& opts) {
  auto* fakes = FakeDevicesFor(args, session);
  if (!fakes)
    return {};
  int vendor_id = 0, product_id = 0;
  std::string name = "Fake USB", serial;
  opts.Get("vendorId", &vendor_id);
  opts.Get("productId", &product_id);
  opts.Get("productName", &name);
  opts.Get("serialNumber", &serial);
  return fakes->usb().AddDevice(vendor_id, product_id, name, serial);
}

void RemoveFakeUsbDevice(gin::Arguments* args,
                         v8::Local<v8::Value> session,
                         const std::string& guid) {
  if (auto* fakes = FakeDevicesFor(args, session))
    fakes->usb().RemoveDevice(guid);
}

std::string AddFakeSerialPort(gin::Arguments* args,
                              v8::Local<v8::Value> session,
                              const gin_helper::Dictionary& opts) {
  auto* fakes = FakeDevicesFor(args, session);
  if (!fakes)
    return {};
  int vendor_id = 0, product_id = 0;
  std::string path = "/dev/ttyFAKE0", display_name, serial;
  opts.Get("path", &path);
  opts.Get("displayName", &display_name);
  opts.Get("vendorId", &vendor_id);
  opts.Get("productId", &product_id);
  opts.Get("serialNumber", &serial);
  return fakes->serial().AddPort(path, display_name, vendor_id, product_id,
                                 serial);
}

void SetFakeSerialPortConnected(gin::Arguments* args,
                                v8::Local<v8::Value> session,
                                const std::string& token,
                                bool connected) {
  if (auto* fakes = FakeDevicesFor(args, session))
    fakes->serial().SetPortConnected(token, connected);
}

// Number of device connections currently open through the fakes.
int FakeDeviceOpenCount(gin::Arguments* args,
                        v8::Local<v8::Value> session,
                        const std::string& kind) {
  auto* fakes = FakeDevicesFor(args, session);
  if (!fakes)
    return 0;
  if (kind == "hid")
    return fakes->hid().OpenConnectionCount();
  if (kind == "usb")
    return fakes->usb().OpenConnectionCount();
  if (kind == "serial")
    return fakes->serial().OpenConnectionCount();
  return 0;
}

void RemoveFakeSerialPort(gin::Arguments* args,
                          v8::Local<v8::Value> session,
                          const std::string& token) {
  if (auto* fakes = FakeDevicesFor(args, session))
    fakes->serial().RemovePort(token);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("log", &Log);
  dict.SetMethod("useFakeDeviceManagers", &UseFakeDeviceManagers);
  dict.SetMethod("addFakeHidDevice", &AddFakeHidDevice);
  dict.SetMethod("removeFakeHidDevice", &RemoveFakeHidDevice);
  dict.SetMethod("addFakeUsbDevice", &AddFakeUsbDevice);
  dict.SetMethod("removeFakeUsbDevice", &RemoveFakeUsbDevice);
  dict.SetMethod("addFakeSerialPort", &AddFakeSerialPort);
  dict.SetMethod("removeFakeSerialPort", &RemoveFakeSerialPort);
  dict.SetMethod("setFakeSerialPortConnected", &SetFakeSerialPortConnected);
  dict.SetMethod("fakeDeviceOpenCount", &FakeDeviceOpenCount);
  dict.SetMethod("getLoggingDestination", &GetLoggingDestination);
  dict.SetMethod("isPlatformCaretBrowsingEnabled",
                 &IsPlatformCaretBrowsingEnabled);
  dict.SetMethod("simulateNetworkServiceCrash", &SimulateNetworkServiceCrash);
  dict.SetMethod("simulatePowerEvent", &SimulatePowerEvent);
  dict.SetMethod("holdRepeatingCallbackForTesting",
                 &HoldRepeatingCallbackForTesting);
  dict.SetMethod("copyHeldRepeatingCallbackForTesting",
                 &CopyHeldRepeatingCallbackForTesting);
  dict.SetMethod("invokeHeldRepeatingCallbackForTesting",
                 &InvokeHeldRepeatingCallbackForTesting);
  dict.SetMethod("invokeCopiedRepeatingCallbackForTesting",
                 &InvokeCopiedRepeatingCallbackForTesting);
  dict.SetMethod("clearPrimaryHeldRepeatingCallbackForTesting",
                 &ClearPrimaryHeldRepeatingCallbackForTesting);
  dict.SetMethod("getHeldRepeatingCallbackCountForTesting",
                 &GetHeldRepeatingCallbackCountForTesting);
  dict.SetMethod("holdOnceCallbackForTesting", &HoldOnceCallbackForTesting);
  dict.SetMethod("invokeHeldOnceCallbackForTesting",
                 &InvokeHeldOnceCallbackForTesting);
  dict.SetMethod("clearHeldCallbacksForTesting", &ClearHeldCallbacksForTesting);
  dict.SetMethod("holdPromiseForTesting", &HoldPromiseForTesting);
  dict.SetMethod("flushPendingWindowStateSaves", &FlushPendingWindowStateSaves);
  dict.SetMethod("commitPendingLocalStateWrites",
                 &CommitPendingLocalStateWrites);
  dict.SetMethod("clearHeldPromiseForTesting", &ClearHeldPromiseForTesting);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_testing, Initialize)
#endif
