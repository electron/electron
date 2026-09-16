// Copyright (c) 2026 Mitchell Cohen.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/linux/launcher_entry.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "components/dbus/utils/variant.h"
#include "components/dbus/utils/write_value.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "shell/common/platform_util.h"

namespace electron::launcher_entry {

namespace {

constexpr char kLauncherEntryInterface[] = "com.canonical.Unity.LauncherEntry";
constexpr char kLauncherEntryPath[] = "/com/canonical/unity/launcherentry";

using Properties = std::map<std::string, dbus_utils::Variant>;

std::optional<std::string> GetAppUri() {
  std::optional<std::string> desktop_id = platform_util::GetDesktopName();
  if (!desktop_id || desktop_id->empty())
    return std::nullopt;
  return "application://" + *desktop_id;
}

void ConnectAndSend(scoped_refptr<dbus::Bus> bus,
                    std::unique_ptr<dbus::Signal> signal) {
  if (!bus->Connect())
    return;
  uint32_t serial = 0;
  bus->Send(signal->raw_message(), &serial);
}

bool EmitUpdate(const Properties& properties) {
  std::optional<std::string> app_uri = GetAppUri();
  if (!app_uri)
    return false;

  auto signal =
      std::make_unique<dbus::Signal>(kLauncherEntryInterface, "Update");
  CHECK(signal->SetPath(dbus::ObjectPath(kLauncherEntryPath)));
  dbus::MessageWriter writer(signal.get());
  writer.AppendString(*app_uri);
  dbus_utils::WriteValue(writer, properties);

  scoped_refptr<dbus::Bus> bus = dbus_thread_linux::GetSharedSessionBus();
  bus->GetDBusTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&ConnectAndSend, bus, std::move(signal)));
  return true;
}

}  // namespace

bool SetBadgeCount(int64_t count) {
  Properties props;
  props.emplace("count", dbus_utils::Variant::Wrap<"x">(count));
  props.emplace("count-visible", dbus_utils::Variant::Wrap<"b">(count != 0));
  return EmitUpdate(props);
}

bool SetProgress(double progress) {
  Properties props;
  props.emplace("progress", dbus_utils::Variant::Wrap<"d">(progress));
  props.emplace("progress-visible", dbus_utils::Variant::Wrap<"b">(
                                        progress > 0.0 && progress < 1.0));
  return EmitUpdate(props);
}

}  // namespace electron::launcher_entry
