// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_NOTIFICATION_TYPES_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_NOTIFICATION_TYPES_H_

#include "build/build_config.h"
#include "extensions/browser/notification_types.h"

#define PREVIOUS_END extensions::NOTIFICATION_EXTENSIONS_END

namespace atom {

// NotificationService &c. are deprecated (https://crbug.com/268984).
// Don't add any new notification types, and migrate existing uses of the
// notification types below to observers.
enum NotificationType {
  NOTIFICATION_ATOM_START = PREVIOUS_END,

  // Sent when an extension should be disabled. The details are an extension id
  NOTIFICATION_DISABLE_USER_EXTENSION_REQUEST = NOTIFICATION_ATOM_START,

  // Sent when an extension should be enabled. The details are an extension id
  NOTIFICATION_ENABLE_USER_EXTENSION_REQUEST,

  // Sent when an extension should be uninstalled.
  // This happens during an update process.
  NOTIFICATION_EXTENSION_UNINSTALL_REQUEST
};

}  // namespace atom

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_NOTIFICATION_TYPES_H_
