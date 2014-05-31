// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/gtk/status_icon.h"

namespace atom {

// static
TrayIcon* TrayIcon::Create() {
  return new StatusIcon;
}

}  // namespace atom
