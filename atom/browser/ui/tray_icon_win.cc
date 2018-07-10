// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/notify_icon.h"
#include "atom/browser/ui/win/notify_icon_host.h"

namespace atom {

// static
TrayIcon* TrayIcon::Create() {
  static NotifyIconHost host;
  return host.CreateNotifyIcon();
}

}  // namespace atom
