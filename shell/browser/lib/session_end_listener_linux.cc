// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/lib/session_end_listener_linux.h"

#include <string>
#include <tuple>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "components/dbus/xdg/inhibit_session_monitor.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "shell/browser/browser.h"
#include "shell/browser/lib/logind_utils.h"

namespace {

const char kPrepareForShutdownSignal[] = "PrepareForShutdown";
const char kInhibitMethod[] = "Inhibit";

}  // namespace

namespace electron {

SessionEndListenerLinux::SessionEndListenerLinux(
    scoped_refptr<dbus::Bus> system_bus,
    scoped_refptr<dbus::Bus> session_bus,
    base::RepeatingClosure on_quit)
    : system_bus_(std::move(system_bus)),
      session_bus_(std::move(session_bus)),
      on_quit_(std::move(on_quit)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ConnectToLogind();

  if (auto* browser = Browser::Get()) {
    browser->SetQuitCancelledCallback(
        base::BindRepeating(&SessionEndListenerLinux::OnQuitCancelled,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  if (session_bus_) {
    // Does nothing when the portal is unavailable.
    portal_monitor_ = std::make_unique<dbus_xdg::InhibitSessionMonitor>(
        session_bus_,
        base::BindOnce(&SessionEndListenerLinux::OnSessionEnding,
                       weak_ptr_factory_.GetWeakPtr(), Source::kPortal));
  }
}

SessionEndListenerLinux::~SessionEndListenerLinux() = default;

void SessionEndListenerLinux::SetShouldQuitHandler(
    base::RepeatingCallback<bool()> should_quit) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  should_quit_ = std::move(should_quit);
}

// static
const char* SessionEndListenerLinux::SourceName(Source source) {
  switch (source) {
    case Source::kLogind:
      return "logind";
    case Source::kPortal:
      return "portal";
  }
  NOTREACHED();
}

void SessionEndListenerLinux::ConnectToLogind() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!system_bus_) {
    return;
  }

  const auto weak_this = weak_ptr_factory_.GetWeakPtr();
  logind_proxy_ = system_bus_->GetObjectProxy(
      logind::kServiceName, dbus::ObjectPath(logind::kObjectPath));

  dbus_utils::ConnectToSignal<"b">(
      logind_proxy_, logind::kManagerInterface, kPrepareForShutdownSignal,
      base::BindRepeating(&SessionEndListenerLinux::OnPrepareForShutdown,
                          weak_this),
      base::BindOnce(&SessionEndListenerLinux::OnPrepareForShutdownConnected,
                     weak_this));

  logind_proxy_->WaitForServiceToBeAvailable(
      base::BindOnce(&SessionEndListenerLinux::OnLogindAvailable, weak_this));
}

void SessionEndListenerLinux::OnPrepareForShutdownConnected(
    const std::string& interface_name,
    const std::string& signal_name,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!success) {
    VLOG(1) << "Failed to connect to " << interface_name << "." << signal_name;
    // Don't make logind wait for a process that can't hear the shutdown.
    logind_proxy_ = nullptr;
    return;
  }
  RequestInhibitor();
}

void SessionEndListenerLinux::OnLogindAvailable(bool available) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!available) {
    VLOG(1) << logind::kServiceName << " not available";
    return;
  }
  // Covers a logind that appeared after the signal connection.
  RequestInhibitor();
}

void SessionEndListenerLinux::RequestInhibitor() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!logind_proxy_ || inhibitor_fd_.is_valid() || inhibitor_requested_) {
    return;
  }
  inhibitor_requested_ = true;

  // A delay lock: logind waits for this process up to InhibitDelayMaxSec.
  dbus_utils::CallMethod<"ssss", "h">(
      logind_proxy_, logind::kManagerInterface, kInhibitMethod,
      base::BindOnce(&SessionEndListenerLinux::OnInhibitResponse,
                     weak_ptr_factory_.GetWeakPtr()),
      "shutdown", logind::GetExecutableBaseName(), "Ensure a clean shutdown",
      "delay");
}

void SessionEndListenerLinux::OnInhibitResponse(
    dbus_utils::CallMethodResultSig<"h"> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  inhibitor_requested_ = false;
  if (!result.has_value()) {
    VLOG(1) << "Failed to acquire a logind shutdown delay inhibitor";
    return;
  }
  inhibitor_fd_ = std::move(std::get<0>(result.value()));
}

void SessionEndListenerLinux::OnPrepareForShutdown(
    dbus_utils::ConnectToSignalResultSig<"b"> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!result.has_value()) {
    return;
  }

  // Cancelled or failed: the next announcement is a new session end. A quit
  // still in flight belongs to the cancelled one, so cancelling that quit must
  // not drop the lock taken for the next.
  if (!std::get<0>(result.value())) {
    ended_ = false;
    quit_started_ = false;
    RequestInhibitor();
    return;
  }

  OnSessionEnding(Source::kLogind);
}

void SessionEndListenerLinux::OnSessionEnding(Source source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (ended_) {
    return;
  }
  // Set before asking: by the time a second source reports the same session
  // end, the app may have dropped its listener. Re-asking would then turn a
  // delay into a quit.
  ended_ = true;

  VLOG(1) << "Session ending, reported by " << SourceName(source);

  if (should_quit_ && !should_quit_.Run()) {
    VLOG(1) << "The app is handling the session end; keeping the delay lock";
    return;
  }

  if (on_quit_) {
    quit_started_ = true;
    on_quit_.Run();
  }
}

void SessionEndListenerLinux::OnQuitCancelled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Don't make logind wait for an app that has decided not to exit.
  if (!quit_started_)
    return;
  quit_started_ = false;
  inhibitor_fd_.reset();
  inhibitor_requested_ = false;
}

}  // namespace electron
