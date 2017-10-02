// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <stdlib.h>

#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "atom/common/atom_version.h"
#include "brightray/common/application_info.h"

#if defined(USE_X11)
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "chrome/browser/ui/libgtkui/unity_service.h"
#endif

namespace atom {

void Browser::Focus() {
  // Focus on the first visible window.
  for (const auto& window : WindowList::GetWindows()) {
    if (window->IsVisible()) {
      window->Focus(true);
      break;
    }
  }
}

void Browser::AddRecentDocument(const base::FilePath& path) {
}

void Browser::ClearRecentDocuments() {
}

void Browser::SetAppUserModelID(const base::string16& name) {
}

const char kXdgSettings[] = "xdg-settings";
const char kXdgSettingsDefaultBrowser[] = "default-web-browser";
const char kXdgSettingsDefaultSchemeHandler[] = "default-url-scheme-handler";

// TODO(codebytere): handle/replace GetChromeVersionOfScript
bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                         mate::Arguments* args) {
  #if defined(OS_CHROMEOS)
    return true;
  #else
    std::unique_ptr<base::Environment> env(base::Environment::Create());

    std::vector<std::string> argv;
    argv.push_back(kXdgSettings);
    argv.push_back("set");
    if (protocol.empty()) {
     argv.push_back(kXdgSettingsDefaultBrowser);
    } else {
     argv.push_back(kXdgSettingsDefaultSchemeHandler);
     argv.push_back(protocol);
    }
    argv.push_back(libgtkui::GetDesktopName(env.get()));

    int exit_code;
    bool ran_ok = LaunchXdgUtility(argv, &exit_code);
    if (ran_ok && exit_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
     if (GetChromeVersionOfScript(kXdgSettings, &argv[0])) {
       ran_ok = LaunchXdgUtility(argv, &exit_code);
     }
    }

    return ran_ok && exit_code == EXIT_SUCCESS;
  #endif
}

// TODO(codebytere): handle/replace GetChromeVersionOfScript
bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      mate::Arguments* args) {
  #if defined(OS_CHROMEOS)
    return UNKNOWN_DEFAULT;
  #else
    base::ThreadRestrictions::AssertIOAllowed();

    std::unique_ptr<base::Environment> env(base::Environment::Create());

    std::vector<std::string> argv;
    argv.push_back(kXdgSettings);
    argv.push_back("check");
    if (protocol.empty()) {
      argv.push_back(kXdgSettingsDefaultBrowser);
    } else {
      argv.push_back(kXdgSettingsDefaultSchemeHandler);
      argv.push_back(protocol);
    }
    argv.push_back(shell_integration_linux::GetDesktopName(env.get()));

    std::string reply;
    int success_code;
    bool ran_ok = base::GetAppOutputWithExitCode(base::CommandLine(argv),
    &reply, &success_code);
    if (ran_ok && success_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
      if (GetChromeVersionOfScript(kXdgSettings, &argv[0])) {
        ran_ok = base::GetAppOutputWithExitCode(base::CommandLine(argv), &reply,
                                                &success_code);
      }
    }

    if (!ran_ok || success_code != EXIT_SUCCESS) {
      // xdg-settings failed: we can't determine or set the default browser.
      return UNKNOWN_DEFAULT;
    }

    // Allow any reply that starts with "yes".
    return base::StartsWith(reply, "yes", base::CompareCase::SENSITIVE)
               ? IS_DEFAULT
               : NOT_DEFAULT;
  #endif
}

// TODO(codebytere): implement method with xdgsettings
bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            mate::Arguments* args) {
  return false;
}

bool Browser::SetBadgeCount(int count) {
  if (IsUnityRunning()) {
    unity::SetDownloadCount(count);
    badge_count_ = count;
    return true;
  } else {
    return false;
  }
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
}

Browser::LoginItemSettings Browser::GetLoginItemSettings(
    const LoginItemSettings& options) {
  return LoginItemSettings();
}

std::string Browser::GetExecutableFileVersion() const {
  return brightray::GetApplicationVersion();
}

std::string Browser::GetExecutableFileProductName() const {
  return brightray::GetApplicationName();
}

bool Browser::IsUnityRunning() {
  return unity::IsRunning();
}

}  // namespace atom
