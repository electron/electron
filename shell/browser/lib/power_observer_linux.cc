// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include "shell/browser/lib/power_observer_linux.h"

#include <unistd.h>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"
#include "shell/browser/lib/logind_utils.h"
#include "shell/common/uv_includes.h"

namespace electron {

PowerObserverLinux::PowerObserverLinux(
    base::PowerSuspendObserver* suspend_observer)
    : suspend_observer_(suspend_observer),
      lock_owner_name_(logind::GetExecutableBaseName()) {
  if (!bluez::BluezDBusManager::IsInitialized())
    bluez::DBusBluezManagerWrapperLinux::Initialize();

  auto bus = dbus_thread_linux::GetSharedSystemBus();
  if (!bus) {
    LOG(WARNING) << "Failed to get system bus connection";
    return;
  }

  // set up the logind proxy

  const auto weakThis = weak_ptr_factory_.GetWeakPtr();

  logind_ = bus->GetObjectProxy(logind::kServiceName,
                                dbus::ObjectPath(logind::kObjectPath));
  logind_->ConnectToSignal(
      logind::kManagerInterface, "PrepareForSleep",
      base::BindRepeating(&PowerObserverLinux::OnPrepareForSleep, weakThis),
      base::BindRepeating(&PowerObserverLinux::OnSignalConnected, weakThis));
  logind_->WaitForServiceToBeAvailable(base::BindRepeating(
      &PowerObserverLinux::OnLoginServiceAvailable, weakThis));
}

PowerObserverLinux::~PowerObserverLinux() = default;

void PowerObserverLinux::OnLoginServiceAvailable(bool service_available) {
  if (!service_available) {
    LOG(WARNING) << logind::kServiceName << " not available";
    return;
  }
  // Take sleep inhibit lock
  BlockSleep();
}

void PowerObserverLinux::BlockSleep() {
  dbus::MethodCall sleep_inhibit_call(logind::kManagerInterface, "Inhibit");
  dbus::MessageWriter inhibit_writer(&sleep_inhibit_call);
  inhibit_writer.AppendString("sleep");  // what
  // Use the executable name as the lock owner, which will list rebrands of the
  // electron executable as separate entities.
  inhibit_writer.AppendString(lock_owner_name_);                      // who
  inhibit_writer.AppendString("Application cleanup before suspend");  // why
  inhibit_writer.AppendString("delay");                               // mode
  logind_->CallMethod(&sleep_inhibit_call,
                      dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                      base::BindOnce(&PowerObserverLinux::OnInhibitResponse,
                                     weak_ptr_factory_.GetWeakPtr()));
}

void PowerObserverLinux::UnblockSleep() {
  sleep_lock_.reset();
}

void PowerObserverLinux::OnInhibitResponse(dbus::Response* response) {
  if (response != nullptr) {
    dbus::MessageReader reader(response);
    reader.PopFileDescriptor(&sleep_lock_);
  }
}

void PowerObserverLinux::OnPrepareForSleep(dbus::Signal* signal) {
  dbus::MessageReader reader(signal);
  bool suspending;
  if (!reader.PopBool(&suspending)) {
    LOG(ERROR) << "Invalid signal: " << signal->ToString();
    return;
  }
  if (suspending) {
    suspend_observer_->OnSuspend();

    UnblockSleep();
  } else {
    BlockSleep();

    suspend_observer_->OnResume();
  }
}

void PowerObserverLinux::OnSignalConnected(const std::string& /*interface*/,
                                           const std::string& signal,
                                           bool success) {
  LOG_IF(WARNING, !success) << "Failed to connect to " << signal;
}

}  // namespace electron
