// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_

#include <map>
#include <string>
#include <vector>

#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"
#include "electron/buildflags/buildflags.h"
#include "third_party/blink/public/mojom/v8_cache_options.mojom-forward.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom-forward.h"

namespace base {
class CommandLine;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

// Stores and applies the preferences of WebContents.
class WebContentsPreferences
    : public content::WebContentsUserData<WebContentsPreferences> {
 public:
  // Get self from WebContents.
  static WebContentsPreferences* From(content::WebContents* web_contents);

  WebContentsPreferences(content::WebContents* web_contents,
                         const gin_helper::Dictionary& web_preferences);
  ~WebContentsPreferences() override;

  // disable copy
  WebContentsPreferences(const WebContentsPreferences&) = delete;
  WebContentsPreferences& operator=(const WebContentsPreferences&) = delete;

  void Merge(const gin_helper::Dictionary& new_web_preferences);

  void SetFromDictionary(const gin_helper::Dictionary& new_web_preferences);

  // Append command paramters according to preferences.
  void AppendCommandLineSwitches(base::CommandLine* command_line,
                                 bool is_subframe);

  // Modify the WebPreferences according to preferences.
  void OverrideWebkitPrefs(blink::web_pref::WebPreferences* prefs);

  base::Value* last_preference() { return &last_web_preferences_; }

  bool IsOffscreen() const { return offscreen_; }
  absl::optional<SkColor> GetBackgroundColor() const {
    return background_color_;
  }
  void SetBackgroundColor(absl::optional<SkColor> color) {
    background_color_ = color;
  }
  bool ShouldUsePreferredSizeMode() const {
    return enable_preferred_size_mode_;
  }
  void SetIgnoreMenuShortcuts(bool ignore_menu_shortcuts) {
    ignore_menu_shortcuts_ = ignore_menu_shortcuts;
  }
  bool ShouldIgnoreMenuShortcuts() const { return ignore_menu_shortcuts_; }
  bool SetImageAnimationPolicy(std::string policy);
  bool ShouldDisableHtmlFullscreenWindowResize() const {
    return disable_html_fullscreen_window_resize_;
  }
  bool ShouldDisableDialogs() const { return disable_dialogs_; }
  bool ShouldUseSafeDialogs() const { return safe_dialogs_; }
  bool GetSafeDialogsMessage(std::string* message) const;
  bool ShouldDisablePopups() const { return disable_popups_; }
  bool IsWebSecurityEnabled() const { return web_security_; }
  bool GetPreloadPath(base::FilePath* path) const;
  bool IsSandboxed() const;

 private:
  friend class content::WebContentsUserData<WebContentsPreferences>;
  friend class ElectronBrowserClient;

  // Get WebContents according to process ID.
  static content::WebContents* GetWebContentsFromProcessID(int process_id);

  void Clear();
  void SaveLastPreferences();

  content::WebContents* web_contents_;

  bool plugins_;
  bool experimental_features_;
  bool node_integration_;
  bool node_integration_in_sub_frames_;
  bool node_integration_in_worker_;
  bool disable_html_fullscreen_window_resize_;
  bool webview_tag_;
  absl::optional<bool> sandbox_;
  bool context_isolation_;
  bool javascript_;
  bool images_;
  bool text_areas_are_resizable_;
  bool webgl_;
  bool enable_websql_;
  bool enable_preferred_size_mode_;
  bool web_security_;
  bool allow_running_insecure_content_;
  bool offscreen_;
  bool navigate_on_drag_drop_;
  blink::mojom::AutoplayPolicy autoplay_policy_;
  std::map<std::string, std::u16string> default_font_family_;
  absl::optional<int> default_font_size_;
  absl::optional<int> default_monospace_font_size_;
  absl::optional<int> minimum_font_size_;
  absl::optional<std::string> default_encoding_;
  bool is_webview_;
  std::vector<std::string> custom_args_;
  std::vector<std::string> custom_switches_;
  absl::optional<std::string> enable_blink_features_;
  absl::optional<std::string> disable_blink_features_;
  bool disable_popups_;
  bool disable_dialogs_;
  bool safe_dialogs_;
  absl::optional<std::string> safe_dialogs_message_;
  bool ignore_menu_shortcuts_;
  absl::optional<SkColor> background_color_;
  blink::mojom::ImageAnimationPolicy image_animation_policy_;
  absl::optional<base::FilePath> preload_path_;
  blink::mojom::V8CacheOptions v8_cache_options_;

#if defined(OS_MAC)
  bool scroll_bounce_;
#endif
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  bool spellcheck_;
#endif

  // This is a snapshot of some relevant preferences at the time the renderer
  // was launched.
  base::Value last_web_preferences_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_
