// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/runtime/electron_runtime_api_delegate.h"

#include <string>

#include "build/build_config.h"
#include "extensions/common/api/runtime.h"
#include "shell/browser/extensions/electron_extension_system.h"

using extensions::api::runtime::PlatformInfo;

namespace extensions {

ElectronRuntimeAPIDelegate::ElectronRuntimeAPIDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(browser_context_);
}

ElectronRuntimeAPIDelegate::~ElectronRuntimeAPIDelegate() = default;

void ElectronRuntimeAPIDelegate::AddUpdateObserver(UpdateObserver* observer) {}

void ElectronRuntimeAPIDelegate::RemoveUpdateObserver(
    UpdateObserver* observer) {}

void ElectronRuntimeAPIDelegate::ReloadExtension(
    const std::string& extension_id) {
  static_cast<ElectronExtensionSystem*>(ExtensionSystem::Get(browser_context_))
      ->ReloadExtension(extension_id);
}

bool ElectronRuntimeAPIDelegate::CheckForUpdates(
    const std::string& extension_id,
    const UpdateCheckCallback& callback) {
  return false;
}

void ElectronRuntimeAPIDelegate::OpenURL(const GURL& uninstall_url) {}

bool ElectronRuntimeAPIDelegate::GetPlatformInfo(PlatformInfo* info) {
  // TODO(nornagon): put useful information here.
#if defined(OS_LINUX)
  info->os = api::runtime::PLATFORM_OS_LINUX;
#endif
  return true;
}  // namespace extensions

bool ElectronRuntimeAPIDelegate::RestartDevice(std::string* error_message) {
  *error_message = "Restart is not supported in Electron";
  return false;
}

}  // namespace extensions
