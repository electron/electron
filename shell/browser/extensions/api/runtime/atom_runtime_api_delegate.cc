// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/runtime/atom_runtime_api_delegate.h"

#include <string>

#include "build/build_config.h"
#include "extensions/common/api/runtime.h"
#include "shell/browser/extensions/atom_extension_system.h"

using extensions::api::runtime::PlatformInfo;

namespace extensions {

AtomRuntimeAPIDelegate::AtomRuntimeAPIDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(browser_context_);
}

AtomRuntimeAPIDelegate::~AtomRuntimeAPIDelegate() = default;

void AtomRuntimeAPIDelegate::AddUpdateObserver(UpdateObserver* observer) {}

void AtomRuntimeAPIDelegate::RemoveUpdateObserver(UpdateObserver* observer) {}

void AtomRuntimeAPIDelegate::ReloadExtension(const std::string& extension_id) {
  static_cast<AtomExtensionSystem*>(ExtensionSystem::Get(browser_context_))
      ->ReloadExtension(extension_id);
}

bool AtomRuntimeAPIDelegate::CheckForUpdates(
    const std::string& extension_id,
    const UpdateCheckCallback& callback) {
  return false;
}

void AtomRuntimeAPIDelegate::OpenURL(const GURL& uninstall_url) {}

bool AtomRuntimeAPIDelegate::GetPlatformInfo(PlatformInfo* info) {
  // TODO(nornagon): put useful information here.
#if defined(OS_LINUX)
  info->os = api::runtime::PLATFORM_OS_LINUX;
#endif
  return true;
}  // namespace extensions

bool AtomRuntimeAPIDelegate::RestartDevice(std::string* error_message) {
  *error_message = "Restart is not supported in Electron";
  return false;
}

}  // namespace extensions
