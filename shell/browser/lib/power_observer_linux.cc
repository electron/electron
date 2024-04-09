// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include "shell/browser/lib/power_observer_linux.h"

#include <unistd.h>
#include <uv.h>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/bluez_dbus_thread_manager.h"
#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"

namespace {

const char kLogindServiceName[] = "org.freedesktop.login1";
const char kLogindObjectPath[] = "/org/freedesktop/login1";
const char kLogindManagerInterface[] = "org.freedesktop.login1.Manager";

base::FilePath::StringType GetExecutableBaseName() {
  return base::CommandLine::ForCurrentProcess()
      ->GetProgram()
      .BaseName()
      .value();
}

}  // namespace

namespace electron {

PowerObserverLinux::PowerObserverLinux(
    base::PowerSuspendObserver* suspend_observer)
    : suspend_observer_(suspend_observer),
      lock_owner_name_(GetExecutableBaseName()) {
  if (!bluez::BluezDBusManager::IsInitialized())
    bluez::DBusBluezManagerWrapperLinux::Initialize();

  auto* bus = bluez::BluezDBusThreadManager::Get()->GetSystemBus();
  if (!bus) {
    LOG(WARNING) << "Failed to get system bus connection";
    return;
  }

  // set up the logind proxy

  const auto weakThis = weak_ptr_factory_.GetWeakPtr();

  logind_ = bus->GetObjectProxy(kLogindServiceName,
                                dbus::ObjectPath(kLogindObjectPath));
  logind_->ConnectToSignal(
      kLogindManagerInterface, "PrepareForShutdown",
      base::BindRepeating(&PowerObserverLinux::OnPrepareForShutdown, weakThis),
      base::BindRepeating(&PowerObserverLinux::OnSignalConnected, weakThis));
  logind_->ConnectToSignal(
      kLogindManagerInterface, "PrepareForSleep",
      base::BindRepeating(&PowerObserverLinux::OnPrepareForSleep, weakThis),
      base::BindRepeating(&PowerObserverLinux::OnSignalConnected, weakThis));
  logind_->WaitForServiceToBeAvailable(base::BindRepeating(
      &PowerObserverLinux::OnLoginServiceAvailable, weakThis));
}

PowerObserverLinux::~PowerObserverLinux() = default;

void PowerObserverLinux::OnLoginServiceAvailable(bool service_available) {
  if (!service_available) {
    LOG(WARNING) << kLogindServiceName << " not available";
    return;
  }
  // Take sleep inhibit lock
  BlockSleep();
}

void PowerObserverLinux::BlockSleep() {
  dbus::MethodCall sleep_inhibit_call(kLogindManagerInterface, "Inhibit");
  dbus::MessageWriter inhibit_writer(&sleep_inhibit_call);
  inhibit_writer.AppendString("sleep");  // what
  // Use the executable name as the lock owner, which will list rebrands of the
  // electron executable as separate entities.
  inhibit_writer.AppendString(lock_owner_name_);                      // who
  inhibit_writer.AppendString("Application cleanup before suspend");  // why
  inhibit_writer.AppendString("delay");                               // mode
  logind_->CallMethod(
      &sleep_inhibit_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&PowerObserverLinux::OnInhibitResponse,
                     weak_ptr_factory_.GetWeakPtr(), &sleep_lock_));
}

void PowerObserverLinux::UnblockSleep() {
  sleep_lock_.reset();
}

void PowerObserverLinux::BlockShutdown() {
  if (shutdown_lock_.is_valid()) {
    LOG(WARNING) << "Trying to subscribe to shutdown multiple times";
    return;
  }
  dbus::MethodCall shutdown_inhibit_call(kLogindManagerInterface, "Inhibit");
  dbus::MessageWriter inhibit_writer(&shutdown_inhibit_call);
  inhibit_writer.AppendString("shutdown");                 // what
  inhibit_writer.AppendString(lock_owner_name_);           // who
  inhibit_writer.AppendString("Ensure a clean shutdown");  // why
  inhibit_writer.AppendString("delay");                    // mode
  logind_->CallMethod(
      &shutdown_inhibit_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&PowerObserverLinux::OnInhibitResponse,
                     weak_ptr_factory_.GetWeakPtr(), &shutdown_lock_));
}

void PowerObserverLinux::UnblockShutdown() {
  if (!shutdown_lock_.is_valid()) {
    LOG(WARNING)
        << "Trying to unsubscribe to shutdown without being subscribed";
    return;
  }
  shutdown_lock_.reset();
}

void PowerObserverLinux::SetShutdownHandler(
    base::RepeatingCallback<bool()> handler) {
  // In order to delay system shutdown when e.preventDefault() is invoked
  // on a powerMonitor 'shutdown' event, we need an org.freedesktop.login1
  // shutdown delay lock. For more details see the "Taking Delay Locks"
  // section of https://www.freedesktop.org/wiki/Software/systemd/inhibit/
  if (handler && !should_shutdown_) {
    BlockShutdown();
  } else if (!handler && should_shutdown_) {
    UnblockShutdown();
  }
  should_shutdown_ = std::move(handler);
}

void PowerObserverLinux::OnInhibitResponse(base::ScopedFD* scoped_fd,
                                           dbus::Response* response) {
  if (response != nullptr) {
    dbus::MessageReader reader(response);
    reader.PopFileDescriptor(scoped_fd);
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

void PowerObserverLinux::OnPrepareForShutdown(dbus::Signal* signal) {
  dbus::MessageReader reader(signal);
  bool shutting_down;
  if (!reader.PopBool(&shutting_down)) {
    LOG(ERROR) << "Invalid signal: " << signal->ToString();
    return;
  }
  if (shutting_down) {
    if (!should_shutdown_ || should_shutdown_.Run()) {
      // The user didn't try to prevent shutdown. Release the lock and allow the
      // shutdown to continue normally.
      shutdown_lock_.reset();
    }
  }
}

void PowerObserverLinux::OnSignalConnected(const std::string& /*interface*/,
                                           const std::string& signal,
                                           bool success) {
  LOG_IF(WARNING, !success) << "Failed to connect to " << signal;
}

}  // namespace electron
