// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/runtime/atom_runtime_api_delegate.h"

#include <string>

#include "atom/browser/extensions/atom_extension_system.h"
#include "build/build_config.h"
#include "extensions/common/api/runtime.h"

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/power_manager_client.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#endif

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
#if defined(OS_CHROMEOS)
  info->os = api::runtime::PLATFORM_OS_CROS;
#elif defined(OS_LINUX)
  info->os = api::runtime::PLATFORM_OS_LINUX;
#endif
  return true;
}

bool AtomRuntimeAPIDelegate::RestartDevice(std::string* error_message) {
// We allow chrome.runtime.restart() to request a device restart on ChromeOS.
#if defined(OS_CHROMEOS)
  chromeos::PowerManagerClient::Get()->RequestRestart(
      power_manager::REQUEST_RESTART_OTHER, "AppShell chrome.runtime API");
  return true;
#else
  *error_message = "Restart is only supported on ChromeOS.";
  return false;
#endif
}

}  // namespace extensions
