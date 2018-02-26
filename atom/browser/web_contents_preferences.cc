// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_preferences.h"

#include <algorithm>
#include <string>
#include <vector>

#include "atom/browser/native_window.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "cc/base/switches.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/web_preferences.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"

#if defined(OS_WIN)
#include "ui/gfx/switches.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::WebContentsPreferences);

namespace atom {

// static
std::vector<WebContentsPreferences*> WebContentsPreferences::instances_;

WebContentsPreferences::WebContentsPreferences(
    content::WebContents* web_contents,
    const mate::Dictionary& web_preferences)
    : web_contents_(web_contents) {
  v8::Isolate* isolate = web_preferences.isolate();
  mate::Dictionary copied(isolate, web_preferences.GetHandle()->Clone());
  // Following fields should not be stored.
  copied.Delete("embedder");
  copied.Delete("isGuest");
  copied.Delete("session");

  mate::ConvertFromV8(isolate, copied.GetHandle(), &web_preferences_);
  web_contents->SetUserData(UserDataKey(), base::WrapUnique(this));

  instances_.push_back(this);
}

WebContentsPreferences::~WebContentsPreferences() {
  instances_.erase(
      std::remove(instances_.begin(), instances_.end(), this),
      instances_.end());
}

void WebContentsPreferences::Merge(const base::DictionaryValue& extend) {
  web_preferences_.MergeDictionary(&extend);
}

// static
content::WebContents* WebContentsPreferences::GetWebContentsFromProcessID(
    int process_id) {
  for (WebContentsPreferences* preferences : instances_) {
    content::WebContents* web_contents = preferences->web_contents_;
    if (web_contents->GetRenderProcessHost()->GetID() == process_id)
      return web_contents;
  }
  return nullptr;
}

bool GetBooleanAndSetDefault(base::DictionaryValue& web_preferences,
                             const std::string key, bool defaultVal) {
  bool tmp = defaultVal;
  if (!web_preferences.GetBoolean(key, &tmp)) {
    web_preferences.SetBoolean(key, defaultVal);
  }
  return tmp;
}

// static
void WebContentsPreferences::AppendExtraCommandLineSwitches(
    content::WebContents* web_contents, base::CommandLine* command_line) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  base::DictionaryValue& web_preferences = self->web_preferences_;

  // Check if plugins are enabled.
  if (GetBooleanAndSetDefault(web_preferences, "plugins", false))
    command_line->AppendSwitch(switches::kEnablePlugins);

  // Experimental flags.
  if (GetBooleanAndSetDefault(web_preferences, options::kExperimentalFeatures,
                              false))
    command_line->AppendSwitch(
        ::switches::kEnableExperimentalWebPlatformFeatures);

  if (GetBooleanAndSetDefault(web_preferences,
                              options::kExperimentalCanvasFeatures, false))
    command_line->AppendSwitch(::switches::kEnableExperimentalCanvasFeatures);

  // Check if we have node integration specified.
  bool node_integration = GetBooleanAndSetDefault(
                            web_preferences,
                            options::kNodeIntegration,
                            true
  );
  command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                  node_integration ? "true" : "false");

  // Whether to enable node integration in Worker.
  if (GetBooleanAndSetDefault(web_preferences,
                              options::kNodeIntegrationInWorker, false))
    command_line->AppendSwitch(switches::kNodeIntegrationInWorker);

  // Check if webview tag creation is enabled, default to nodeIntegration value.
  // TODO(kevinsawicki): Default to false in 2.0
  bool webview_tag = GetBooleanAndSetDefault(
                        web_preferences,
                        options::kWebviewTag,
                        node_integration
  );
  command_line->AppendSwitchASCII(switches::kWebviewTag,
                                  webview_tag ? "true" : "false");

  // If the `sandbox` option was passed to the BrowserWindow's webPreferences,
  // pass `--enable-sandbox` to the renderer so it won't have any node.js
  // integration.
  if (GetBooleanAndSetDefault(web_preferences, "sandbox", false)) {
    command_line->AppendSwitch(switches::kEnableSandbox);
  } else if (!command_line->HasSwitch(switches::kEnableSandbox)) {
    command_line->AppendSwitch(::switches::kNoSandbox);
  }
  if (GetBooleanAndSetDefault(web_preferences, "nativeWindowOpen", false))
    command_line->AppendSwitch(switches::kNativeWindowOpen);

  // The preload script.
  base::FilePath::StringType preload;
  if (web_preferences.GetString(options::kPreloadScript, &preload)) {
    if (base::FilePath(preload).IsAbsolute())
      command_line->AppendSwitchNative(switches::kPreloadScript, preload);
    else
      LOG(ERROR) << "preload script must have absolute path.";
  } else if (web_preferences.GetString(options::kPreloadURL, &preload)) {
    // Translate to file path if there is "preload-url" option.
    base::FilePath preload_path;
    if (net::FileURLToFilePath(GURL(preload), &preload_path))
      command_line->AppendSwitchPath(switches::kPreloadScript, preload_path);
    else
      LOG(ERROR) << "preload url must be file:// protocol.";
  }

  // Custom args for renderer process
  base::Value* customArgs;
  if ((web_preferences.Get(options::kCustomArgs, &customArgs))
      && (customArgs->is_list())) {
    for (const base::Value& customArg : customArgs->GetList()) {
      if (customArg.is_string()) {
        command_line->AppendArg(customArg.GetString());
      }
    }
  }

  // Run Electron APIs and preload script in isolated world
  if (GetBooleanAndSetDefault(web_preferences, options::kContextIsolation,
                              false))
    command_line->AppendSwitch(switches::kContextIsolation);

  // --background-color.
  std::string color;
  if (web_preferences.GetString(options::kBackgroundColor, &color))
    command_line->AppendSwitchASCII(switches::kBackgroundColor, color);

  // --guest-instance-id, which is used to identify guest WebContents.
  int guest_instance_id = 0;
  if (web_preferences.GetInteger(options::kGuestInstanceID, &guest_instance_id))
    command_line->AppendSwitchASCII(switches::kGuestInstanceID,
                                    base::IntToString(guest_instance_id));

  // Pass the opener's window id.
  int opener_id;
  if (web_preferences.GetInteger(options::kOpenerID, &opener_id))
    command_line->AppendSwitchASCII(switches::kOpenerID,
                                    base::IntToString(opener_id));

#if defined(OS_MACOSX)
  // Enable scroll bounce.
  bool scroll_bounce;
  if (web_preferences.GetBoolean(options::kScrollBounce, &scroll_bounce) &&
      scroll_bounce)
    command_line->AppendSwitch(switches::kScrollBounce);
#endif

  // Custom command line switches.
  const base::ListValue* args;
  if (web_preferences.GetList("commandLineSwitches", &args)) {
    for (size_t i = 0; i < args->GetSize(); ++i) {
      std::string arg;
      if (args->GetString(i, &arg) && !arg.empty())
        command_line->AppendSwitch(arg);
    }
  }

  // Enable blink features.
  std::string blink_features;
  if (web_preferences.GetString(options::kBlinkFeatures, &blink_features))
    command_line->AppendSwitchASCII(::switches::kEnableBlinkFeatures,
                                    blink_features);

  // Disable blink features.
  std::string disable_blink_features;
  if (web_preferences.GetString(options::kDisableBlinkFeatures,
                                &disable_blink_features))
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures,
                                    disable_blink_features);

  if (guest_instance_id) {
    // Webview `document.visibilityState` tracks window visibility so we need
    // to let it know if the window happens to be hidden right now.
    auto manager = WebViewManager::GetWebViewManager(web_contents);
    if (manager) {
      auto embedder = manager->GetEmbedder(guest_instance_id);
      if (embedder) {
        auto* relay = NativeWindowRelay::FromWebContents(web_contents);
        if (relay) {
          auto* window = relay->window.get();
          if (window) {
            const bool visible = window->IsVisible() && !window->IsMinimized();
            if (!visible) {
              command_line->AppendSwitch(switches::kHiddenPage);
            }
          }
        }
      }
    }
  }
}

bool WebContentsPreferences::IsPreferenceEnabled(
    const std::string& attribute_name,
    content::WebContents* web_contents) {
  WebContentsPreferences* self;
  if (!web_contents)
    return false;

  self = FromWebContents(web_contents);
  if (!self)
    return false;

  base::DictionaryValue& web_preferences = self->web_preferences_;
  bool bool_value = false;
  web_preferences.GetBoolean(attribute_name, &bool_value);
  return bool_value;
}

// static
void WebContentsPreferences::OverrideWebkitPrefs(
    content::WebContents* web_contents, content::WebPreferences* prefs) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  prefs->javascript_enabled = GetBooleanAndSetDefault(
    self->web_preferences_, "javascript", true);
  prefs->images_enabled = GetBooleanAndSetDefault(
    self->web_preferences_, "images", true);
  prefs->text_areas_are_resizable = GetBooleanAndSetDefault(
    self->web_preferences_, "textAreasAreResizable", true);
  prefs->experimental_webgl_enabled = GetBooleanAndSetDefault(
    self->web_preferences_, "webgl", true);
  bool webSecurity = GetBooleanAndSetDefault(
    self->web_preferences_, "webSecurity", true);
  prefs->allow_running_insecure_content = !webSecurity;
  prefs->web_security_enabled = webSecurity;
  prefs->allow_running_insecure_content = GetBooleanAndSetDefault(
    self->web_preferences_, "allowRunningInsecureContent", false
  );

  const base::DictionaryValue* fonts = nullptr;
  if (self->web_preferences_.GetDictionary("defaultFontFamily", &fonts)) {
    base::string16 font;
    if (fonts->GetString("standard", &font))
      prefs->standard_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("serif", &font))
      prefs->serif_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("sansSerif", &font))
      prefs->sans_serif_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("monospace", &font))
      prefs->fixed_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("cursive", &font))
      prefs->cursive_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("fantasy", &font))
      prefs->fantasy_font_family_map[content::kCommonScript] = font;
  }
  int size;
  if (self->GetInteger("defaultFontSize", &size))
    prefs->default_font_size = size;
  if (self->GetInteger("defaultMonospaceFontSize", &size))
    prefs->default_fixed_font_size = size;
  if (self->GetInteger("minimumFontSize", &size))
    prefs->minimum_font_size = size;
  std::string encoding;
  if (self->web_preferences_.GetString("defaultEncoding", &encoding))
    prefs->default_encoding = encoding;
}

bool WebContentsPreferences::GetInteger(const std::string& attributeName,
                                        int* intValue) {
  // if it is already an integer, no conversion needed
  if (web_preferences_.GetInteger(attributeName, intValue))
    return true;

  base::string16 stringValue;
  if (web_preferences_.GetString(attributeName, &stringValue))
    return base::StringToInt(stringValue, intValue);

  return false;
}

bool WebContentsPreferences::GetString(const std::string& attribute_name,
                                       std::string* string_value,
                                       content::WebContents* web_contents) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return false;
  return self->web_preferences()->GetString(attribute_name, string_value);
}

}  // namespace atom
