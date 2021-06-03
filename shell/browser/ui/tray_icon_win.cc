// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/notify_icon.h"
#include "shell/browser/ui/win/notify_icon_host.h"

namespace electron {

// static
TrayIcon* TrayIcon::Create(absl::optional<UUID> guid) {
  static NotifyIconHost host;
  return host.CreateNotifyIcon(guid);
}

}  // namespace electron
