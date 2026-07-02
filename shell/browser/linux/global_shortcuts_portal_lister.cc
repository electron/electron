// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/linux/global_shortcuts_portal_lister.h"

#include <memory>
#include <tuple>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "base/unguessable_token.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "components/dbus/xdg/portal.h"
#include "components/dbus/xdg/request.h"
#include "content/public/browser/browser_thread.h"
#include "dbus/bus.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"

namespace electron {

namespace {

constexpr char kPortalServiceName[] = "org.freedesktop.portal.Desktop";
constexpr char kPortalObjectPath[] = "/org/freedesktop/portal/desktop";
constexpr char kGlobalShortcutsInterface[] =
    "org.freedesktop.portal.GlobalShortcuts";
constexpr char kMethodCreateSession[] = "CreateSession";
constexpr char kMethodListShortcuts[] = "ListShortcuts";

using DbusShortcut = std::tuple<std::string, dbus_xdg::Dictionary>;
using DbusShortcuts = std::vector<DbusShortcut>;

template <typename T>
std::optional<T> TakeFromDict(dbus_xdg::Dictionary& dict,
                              const std::string& key) {
  auto it = dict.find(key);
  if (it == dict.end()) {
    return std::nullopt;
  }
  auto result = std::move(it->second).Take<T>();
  dict.erase(it);
  return result;
}

// Services ListShortcuts queries through a single long-lived portal session,
// coalescing concurrent callers into one in-flight query.
//
// The session is deliberately created once and NEVER explicitly closed: on
// KDE, all portal sessions of one application share a single kglobalaccel
// component, and xdg-desktop-portal-kde deactivates the component's actions
// when any of those sessions is closed - so closing a short-lived query
// session would permanently stop the application's registered shortcuts from
// firing. Dropping the D-Bus connection at process exit does not trigger
// that teardown.
class PortalShortcutsLister {
 public:
  static PortalShortcutsLister& GetInstance() {
    static base::NoDestructor<PortalShortcutsLister> instance;
    return *instance;
  }

  PortalShortcutsLister(const PortalShortcutsLister&) = delete;
  PortalShortcutsLister& operator=(const PortalShortcutsLister&) = delete;

  void List(ListPortalGlobalShortcutsCallback callback) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!bus_) {
      bus_ = dbus_thread_linux::GetSharedSessionBus();
    }

    const bool query_in_progress = !pending_callbacks_.empty();
    pending_callbacks_.push_back(std::move(callback));
    if (query_in_progress) {
      // Coalesced into the in-flight query.
      return;
    }

    retried_ = false;
    ArmWatchdog();
    // RequestXdgDesktopPortal() sequences the systemd transient scope setup
    // (or the Registry.Register fallback) before the first portal contact.
    // Contacting the portal earlier would make xdg-desktop-portal resolve
    // and cache an empty app id for this connection, which e.g. KDE then
    // uses to key shortcuts by session token instead of by application -
    // fragmenting approvals and making ListShortcuts return an empty list.
    // The result is cached, so subsequent queries continue immediately.
    dbus_xdg::RequestXdgDesktopPortal(
        bus_.get(),
        // Unretained is safe, this instance is never destroyed.
        base::BindOnce(&PortalShortcutsLister::OnServiceStarted,
                       base::Unretained(this), generation_));
  }

 private:
  friend class base::NoDestructor<PortalShortcutsLister>;

  PortalShortcutsLister() = default;
  ~PortalShortcutsLister() = default;

  void OnServiceStarted(uint64_t generation, uint32_t version) {
    if (generation != generation_) {
      // A watchdog timeout already failed this cycle.
      return;
    }
    ArmWatchdog();

    if (version == 0) {
      Flush(std::nullopt);
      return;
    }

    if (!global_shortcuts_proxy_) {
      global_shortcuts_proxy_ = bus_->GetObjectProxy(
          kPortalServiceName, dbus::ObjectPath(kPortalObjectPath));
    }

    if (session_path_.value().empty()) {
      CreateSession();
    } else {
      IssueListShortcuts();
    }
  }

  void CreateSession() {
    dbus_xdg::Dictionary options;
    options["session_handle_token"] = dbus_utils::Variant::Wrap<"s">(
        "electron_list_shortcuts_" +
        base::UnguessableToken::Create().ToString());
    request_ = std::make_unique<dbus_xdg::Request>(
        bus_, global_shortcuts_proxy_, kGlobalShortcutsInterface,
        kMethodCreateSession, std::move(options),
        // Unretained is safe, this instance is never destroyed.
        base::BindOnce(&PortalShortcutsLister::OnCreateSession,
                       base::Unretained(this), generation_));
  }

  void OnCreateSession(
      uint64_t generation,
      base::expected<dbus_xdg::Dictionary, dbus_xdg::ResponseError> results) {
    if (generation != generation_) {
      return;
    }
    ArmWatchdog();
    request_.reset();

    if (!results.has_value()) {
      Flush(std::nullopt);
      return;
    }

    auto session_handle =
        TakeFromDict<std::string>(*results, "session_handle");
    if (!session_handle) {
      Flush(std::nullopt);
      return;
    }

    // Guard against a misbehaving portal: an invalid object path would
    // CHECK-fail when appended to the ListShortcuts method call.
    dbus::ObjectPath session_path{*session_handle};
    if (!session_path.IsValid()) {
      Flush(std::nullopt);
      return;
    }

    session_path_ = std::move(session_path);
    IssueListShortcuts();
  }

  void IssueListShortcuts() {
    request_ = std::make_unique<dbus_xdg::Request>(
        bus_, global_shortcuts_proxy_, kGlobalShortcutsInterface,
        kMethodListShortcuts, dbus_xdg::Dictionary(),
        // Unretained is safe, this instance is never destroyed.
        base::BindOnce(&PortalShortcutsLister::OnListShortcuts,
                       base::Unretained(this), generation_),
        session_path_);
  }

  void OnListShortcuts(
      uint64_t generation,
      base::expected<dbus_xdg::Dictionary, dbus_xdg::ResponseError> results) {
    if (generation != generation_) {
      return;
    }
    ArmWatchdog();
    request_.reset();

    if (!results.has_value()) {
      // The session may have died with a restarted portal service. Retry
      // once with a fresh session. The dead session is dropped without an
      // explicit Close - see the class comment.
      session_path_ = dbus::ObjectPath();
      if (!retried_) {
        retried_ = true;
        CreateSession();
        return;
      }
      Flush(std::nullopt);
      return;
    }

    auto shortcuts = TakeFromDict<DbusShortcuts>(*results, "shortcuts");
    if (!shortcuts) {
      Flush(std::nullopt);
      return;
    }

    std::vector<PortalGlobalShortcut> parsed;
    parsed.reserve(shortcuts->size());
    for (DbusShortcut& shortcut : *shortcuts) {
      PortalGlobalShortcut& entry = parsed.emplace_back();
      entry.id = std::get<0>(shortcut);
      dbus_xdg::Dictionary& properties = std::get<1>(shortcut);
      entry.description =
          TakeFromDict<std::string>(properties, "description").value_or("");
      entry.trigger_description =
          TakeFromDict<std::string>(properties, "trigger_description")
              .value_or("");
    }
    Flush(std::move(parsed));
  }

  // Bounds every step of a query cycle. dbus_xdg::Request has no timeout on
  // its Response signal wait, so a portal that dies between the method reply
  // and the Response signal would otherwise leave the cycle - and with it
  // every future query, which coalesces into the pending one - stuck forever.
  void ArmWatchdog() {
    watchdog_.Start(FROM_HERE, base::Seconds(30),
                    // Unretained is safe, this instance is never destroyed.
                    base::BindOnce(&PortalShortcutsLister::OnWatchdogTimeout,
                                   base::Unretained(this)));
  }

  void OnWatchdogTimeout() {
    // Invalidate any late callbacks from the abandoned cycle and cancel the
    // in-flight request. The session state is unknown; drop it so the next
    // cycle starts fresh (without an explicit Close - see the class comment).
    ++generation_;
    request_.reset();
    session_path_ = dbus::ObjectPath();
    Flush(std::nullopt);
  }

  void Flush(std::optional<std::vector<PortalGlobalShortcut>> result) {
    watchdog_.Stop();
    ++generation_;
    std::vector<ListPortalGlobalShortcutsCallback> callbacks =
        std::exchange(pending_callbacks_, {});
    for (size_t i = 0; i < callbacks.size(); ++i) {
      if (i + 1 == callbacks.size()) {
        std::move(callbacks[i]).Run(std::move(result));
      } else {
        std::move(callbacks[i]).Run(result);
      }
    }
  }

  scoped_refptr<dbus::Bus> bus_;
  raw_ptr<dbus::ObjectProxy> global_shortcuts_proxy_ = nullptr;
  dbus::ObjectPath session_path_;
  std::unique_ptr<dbus_xdg::Request> request_;
  std::vector<ListPortalGlobalShortcutsCallback> pending_callbacks_;
  bool retried_ = false;
  uint64_t generation_ = 0;
  base::OneShotTimer watchdog_;
};

}  // namespace

void ListPortalGlobalShortcuts(ListPortalGlobalShortcutsCallback callback) {
  PortalShortcutsLister::GetInstance().List(std::move(callback));
}

}  // namespace electron
