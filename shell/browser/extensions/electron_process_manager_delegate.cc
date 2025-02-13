// Copyright 2019 Slack Technologies, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/debug/stack_trace.h"

#include "shell/browser/extensions/electron_process_manager_delegate.h"

#include "base/one_shot_event.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"

namespace extensions {

ElectronProcessManagerDelegate::ElectronProcessManagerDelegate() = default;
ElectronProcessManagerDelegate::~ElectronProcessManagerDelegate() = default;

bool ElectronProcessManagerDelegate::AreBackgroundPagesAllowedForContext(
    content::BrowserContext* context) const {
  return true;
}

bool ElectronProcessManagerDelegate::IsExtensionBackgroundPageAllowed(
    content::BrowserContext* context,
    const Extension& extension) const {
  return true;
}

bool ElectronProcessManagerDelegate::DeferCreatingStartupBackgroundHosts(
    content::BrowserContext* context) const {
  return false;
}

}  // namespace extensions
