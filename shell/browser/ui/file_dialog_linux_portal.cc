// Copyright (c) 2025 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/file_dialog.h"

#include <optional>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/atomic_flag.h"
#include "base/task/lazy_thread_pool_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "components/dbus/utils/check_for_service_and_start.h"
#include "dbus/bus.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "dbus/property.h"

namespace file_dialog {

namespace {

constexpr char kXdgPortalService[] = "org.freedesktop.portal.Desktop";
constexpr char kXdgPortalObject[] = "/org/freedesktop/portal/desktop";
constexpr char kFileChooserInterfaceName[] =
    "org.freedesktop.portal.FileChooser";

// Version 4 includes support for current_folder option to the OpenFile method
// via https://github.com/flatpak/xdg-desktop-portal/commit/71165a5.
uint32_t g_required_portal_version = 3;
uint32_t g_available_portal_version = 0;
constexpr char kXdgPortalRequiredVersionFlag[] = "xdg-portal-required-version";

bool g_portal_available = false;

// Refs
// https://source.chromium.org/chromium/chromium/src/+/main:components/dbus/thread_linux/dbus_thread_linux.cc;l=16-24;drc=1c281d7923af032d78ea7247e99b717dfebeb0b2
base::LazyThreadPoolSingleThreadTaskRunner g_electron_dbus_thread_task_runner =
    LAZY_THREAD_POOL_SINGLE_THREAD_TASK_RUNNER_INITIALIZER(
        base::TaskTraits(base::MayBlock(), base::TaskPriority::USER_BLOCKING),
        base::SingleThreadTaskRunnerThreadMode::SHARED);

struct FileChooserProperties : dbus::PropertySet {
  dbus::Property<uint32_t> version;

  explicit FileChooserProperties(dbus::ObjectProxy* object_proxy)
      : dbus::PropertySet(object_proxy, kFileChooserInterfaceName, {}) {
    RegisterProperty("version", &version);
  }

  ~FileChooserProperties() override = default;
};

base::AtomicFlag* GetAvailabilityTestCompletionFlag() {
  static base::NoDestructor<base::AtomicFlag> flag;
  return flag.get();
}

void CheckPortalAvailabilityOnBusThread() {
  auto* flag = GetAvailabilityTestCompletionFlag();
  if (flag->IsSet())
    return;

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SESSION;
  options.connection_type = dbus::Bus::PRIVATE;
  options.dbus_task_runner = g_electron_dbus_thread_task_runner.Get();
  scoped_refptr<dbus::Bus> bus =
      base::MakeRefCounted<dbus::Bus>(std::move(options));
  dbus_utils::CheckForServiceAndStart(
      bus, kXdgPortalService,
      base::BindOnce(
          [](scoped_refptr<dbus::Bus> bus, base::AtomicFlag* flag,
             std::optional<bool> name_has_owner) {
            if (name_has_owner.value_or(false)) {
              //
              dbus::ObjectPath portal_path(kXdgPortalObject);
              dbus::ObjectProxy* portal =
                  bus->GetObjectProxy(kXdgPortalService, portal_path);
              FileChooserProperties properties(portal);
              if (!properties.GetAndBlock(&properties.version)) {
                LOG(ERROR) << "Failed to read portal version property";
              } else if (properties.version.value() >=
                         g_required_portal_version) {
                g_portal_available = true;
                g_available_portal_version = properties.version.value();
              }
            }
            VLOG(1) << "File chooser portal available: "
                    << (g_portal_available ? "yes" : "no");
            flag->Set();
            bus->ShutdownAndBlock();
            bus.reset();
          },
          bus, flag));
}

}  // namespace

void StartPortalAvailabilityTestInBackground() {
  if (GetAvailabilityTestCompletionFlag()->IsSet())
    return;

  const auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!base::StringToUint(
          cmd->GetSwitchValueASCII(kXdgPortalRequiredVersionFlag),
          &g_required_portal_version)) {
    VLOG(1) << "Unable to parse --xdg-portal-required-version";
  }

  g_electron_dbus_thread_task_runner.Get()->PostTask(
      FROM_HERE, base::BindOnce(&CheckPortalAvailabilityOnBusThread));
}

bool IsPortalAvailable() {
  if (!GetAvailabilityTestCompletionFlag()->IsSet())
    LOG(WARNING) << "Portal availability checked before test was complete";

  return g_portal_available;
}

uint32_t GetPortalVersion() {
  return g_available_portal_version;
}

}  // namespace file_dialog
