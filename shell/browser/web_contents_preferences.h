// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_
#define ELECTRON_SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"
#include "electron/buildflags/buildflags.h"
#include "third_party/blink/public/common/renderer_preferences/renderer_preferences.h"
#include "third_party/blink/public/mojom/v8_cache_options.mojom-forward.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom-forward.h"

namespace base {
class CommandLine;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

// The webPreferences that shape a WebContents' renderer process command line.
// Evaluated from the constructor options as well, before any WebContents
// exists, to decide whether a pre-warmed renderer fits.
struct RendererProcessPreferences {
  static RendererProcessPreferences From(
      const gin_helper::Dictionary& web_preferences);

  RendererProcessPreferences();
  RendererProcessPreferences(const RendererProcessPreferences&);
  RendererProcessPreferences& operator=(const RendererProcessPreferences&);
  ~RendererProcessPreferences();

  void AppendCommandLineSwitches(base::CommandLine* command_line,
                                 bool is_subframe) const;
  // True when a renderer launched ahead of time with just --enable-sandbox
  // matches what AppendCommandLineSwitches() would produce for a main frame.
  bool CanUseSpareRenderer() const;

  // From webPreferences alone; app.enableSandbox() applies on top.
  bool sandboxed = true;
  bool offscreen = false;
  bool experimental_features = false;
  bool node_integration_in_sub_frames = false;
  std::vector<std::string> custom_args;
  std::optional<std::string> enable_blink_features;
  std::optional<std::string> disable_blink_features;
#if BUILDFLAG(IS_MAC)
  bool scroll_bounce = false;
#endif
};

// Stores and applies the preferences of WebContents.
class WebContentsPreferences
    : public content::WebContentsUserData<WebContentsPreferences> {
 public:
  // Get self from WebContents.
  static WebContentsPreferences* From(content::WebContents* web_contents);

  // Whether |web_contents| runs a sandboxed renderer. A WebContents that
  // never went through a BrowserWindow/webContents constructor (extension
  // pages, devtools) has no WebContentsPreferences but still runs the
  // sandboxed renderer (the default since Electron 20), so a null prefs
  // counts as sandboxed. app.enableSandbox() / --enable-sandbox forces every
  // renderer sandboxed regardless of per-WC webPreferences.
  static bool ShouldUseSandbox(content::WebContents* web_contents);

  // The sandbox state a WebContents constructed from |web_preferences| would
  // have, following the same rules as ShouldUseSandbox().
  static bool IsSandboxed(const gin_helper::Dictionary& web_preferences);

  WebContentsPreferences(content::WebContents* web_contents,
                         const gin_helper::Dictionary& web_preferences);
  ~WebContentsPreferences() override;

  // disable copy
  WebContentsPreferences(const WebContentsPreferences&) = delete;
  WebContentsPreferences& operator=(const WebContentsPreferences&) = delete;

  void SetFromDictionary(const gin_helper::Dictionary& new_web_preferences);

  // Append command parameters according to preferences.
  void AppendCommandLineSwitches(base::CommandLine* command_line,
                                 bool is_subframe);

  // Modify the WebPreferences according to preferences.
  void OverrideWebkitPrefs(blink::web_pref::WebPreferences* prefs,
                           blink::RendererPreferences* renderer_prefs);

  const base::Value* last_preference() const { return &last_web_preferences_; }

  bool IsOffscreen() const { return renderer_.offscreen; }
  std::optional<SkColor> GetBackgroundColor() const {
    return background_color_;
  }
  void SetBackgroundColor(std::optional<SkColor> color) {
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
  bool AllowsNodeIntegrationInSubFrames() const {
    return renderer_.node_integration_in_sub_frames;
  }
  bool ShouldDisablePopups() const { return disable_popups_; }
  bool IsWebSecurityEnabled() const { return web_security_; }
  std::optional<base::FilePath> GetPreloadPath() const { return preload_path_; }
  bool ShouldFocusOnNavigation() const { return focus_on_navigation_; }
  bool IsSandboxed() const { return renderer_.sandboxed; }
  bool CanUseSpareRenderer() const { return renderer_.CanUseSpareRenderer(); }

 private:
  friend class content::WebContentsUserData<WebContentsPreferences>;
  friend class ElectronBrowserClient;

  // Get WebContents according to process ID.
  static content::WebContents* GetWebContentsFromProcessID(
      content::ChildProcessId process_id);

  void Clear();
  void SaveLastPreferences();

  // TODO(clavin): refactor to use the WebContents provided by the
  // WebContentsUserData base class instead of storing a duplicate ref
  raw_ptr<content::WebContents> web_contents_;

  RendererProcessPreferences renderer_;
  bool plugins_;
  bool node_integration_;
  bool node_integration_in_worker_;
  bool disable_html_fullscreen_window_resize_;
  bool webview_tag_;
  bool context_isolation_;
  bool javascript_;
  bool images_;
  bool text_areas_are_resizable_;
  bool webgl_;
  bool enable_preferred_size_mode_;
  bool web_security_;
  bool allow_running_insecure_content_;
  bool navigate_on_drag_drop_;
  blink::mojom::AutoplayPolicy autoplay_policy_;
  std::map<std::string, std::u16string> default_font_family_;
  std::optional<int> default_font_size_;
  std::optional<int> default_monospace_font_size_;
  std::optional<int> minimum_font_size_;
  std::optional<std::string> default_encoding_;
  bool is_webview_;
  bool disable_popups_;
  bool disable_dialogs_;
  bool safe_dialogs_;
  std::optional<std::string> safe_dialogs_message_;
  bool ignore_menu_shortcuts_;
  std::optional<SkColor> background_color_;
  blink::mojom::ImageAnimationPolicy image_animation_policy_;
  std::optional<base::FilePath> preload_path_;
  blink::mojom::V8CacheOptions v8_cache_options_;
  bool deprecated_paste_enabled_ = false;
  bool focus_on_navigation_;
  bool disable_wake_locks_;

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
