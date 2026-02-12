// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_
#define ELECTRON_SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_

#include "chrome/browser/ui/webui/accessibility/accessibility_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

// Controls the accessibility web UI page.
class ElectronAccessibilityUI : public content::WebUIController {
 public:
  explicit ElectronAccessibilityUI(content::WebUI* web_ui);
  ~ElectronAccessibilityUI() override;
};

// Manages messages sent from accessibility.js via json.
class ElectronAccessibilityUIMessageHandler
    : public AccessibilityUIMessageHandler {
 public:
  ElectronAccessibilityUIMessageHandler();

  // disable copy
  ElectronAccessibilityUIMessageHandler(
      const ElectronAccessibilityUIMessageHandler&) = delete;
  ElectronAccessibilityUIMessageHandler& operator=(
      const ElectronAccessibilityUIMessageHandler&) = delete;

  void RegisterMessages() final;

  static void RegisterPrefs(user_prefs::PrefRegistrySyncable* registry);

 private:
  void GetRequestTypeAndFilters(const base::DictValue& data,
                                std::string& request_type,
                                std::string& allow,
                                std::string& allow_empty,
                                std::string& deny);

  void RequestNativeUITree(const base::ListValue& args);

  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override;

  // Updates the UI with new data. Called periodically to keep the UI up-to-date
  // while it is visible.
  void OnUpdateDisplayTimer();

  // The last data for display sent to the UI by OnUpdateDisplayTimer.
  base::DictValue last_data_;

  // A timer that runs while the UI is visible to call OnUpdateDisplayTimer.
  base::RepeatingTimer update_display_timer_;
};

#endif  // ELECTRON_SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_
