// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LIB_SESSION_END_LISTENER_LINUX_H_
#define ELECTRON_SHELL_BROWSER_LIB_SESSION_END_LISTENER_LINUX_H_

#include <memory>
#include <string>

#include "base/files/scoped_file.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/dbus/utils/call_method.h"
#include "components/dbus/utils/connect_to_signal.h"
#include "dbus/bus.h"

namespace dbus {
class ObjectProxy;
}  // namespace dbus

namespace dbus_xdg {
class InhibitSessionMonitor;
}  // namespace dbus_xdg

namespace electron {

// Watches for the end of the desktop session on Linux: logind's
// PrepareForShutdown for reboots and power-offs, and the
// org.freedesktop.portal.Inhibit session state monitor for logouts, which
// logind does not announce.
//
// Holds a logind shutdown delay lock for the process lifetime, so logind waits
// for this process instead of tearing the session down around it. The lock is
// released by the kernel when the process exits.
//
// `should_quit` is asked once per session end; returning false means the app is
// handling it and the lock is left alone. Otherwise `on_quit` runs. A logout
// has no equivalent delay primitive, so returning false there only means "do
// not quit yet".
class SessionEndListenerLinux {
 public:
  // Where a session end was observed, for logging and tests.
  enum class Source {
    kLogind,
    kPortal,
  };

  SessionEndListenerLinux(scoped_refptr<dbus::Bus> system_bus,
                          scoped_refptr<dbus::Bus> session_bus,
                          base::RepeatingClosure on_quit);
  ~SessionEndListenerLinux();

  // disable copy
  SessionEndListenerLinux(const SessionEndListenerLinux&) = delete;
  SessionEndListenerLinux& operator=(const SessionEndListenerLinux&) = delete;

  // An empty callback means nothing wants to delay, i.e. quit.
  void SetShouldQuitHandler(base::RepeatingCallback<bool()> should_quit);

 private:
  static const char* SourceName(Source source);

  void ConnectToLogind();
  void OnPrepareForShutdownConnected(const std::string& interface_name,
                                     const std::string& signal_name,
                                     bool success);
  void OnLogindAvailable(bool available);
  void RequestInhibitor();
  void OnInhibitResponse(dbus_utils::CallMethodResultSig<"h"> result);
  void OnPrepareForShutdown(dbus_utils::ConnectToSignalResultSig<"b"> result);
  void OnSessionEnding(Source source);
  void OnQuitCancelled();

  const scoped_refptr<dbus::Bus> system_bus_;
  const scoped_refptr<dbus::Bus> session_bus_;
  raw_ptr<dbus::ObjectProxy> logind_proxy_ = nullptr;
  std::unique_ptr<dbus_xdg::InhibitSessionMonitor> portal_monitor_;

  base::ScopedFD inhibitor_fd_;
  bool inhibitor_requested_ = false;
  base::RepeatingCallback<bool()> should_quit_;
  base::RepeatingClosure on_quit_;

  // One session end is reported by several mechanisms (a reboot ends the
  // session, so the portal reports it too). The first one wins.
  bool ended_ = false;

  // Whether a quit was started for the session end and has not been cancelled.
  bool quit_started_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<SessionEndListenerLinux> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_LIB_SESSION_END_LISTENER_LINUX_H_
