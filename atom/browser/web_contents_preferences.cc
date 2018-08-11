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
#include "content/public/browser/render_frame_host.h"
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

  mate::ConvertFromV8(isolate, copied.GetHandle(), &dict_);
  web_contents->SetUserData(UserDataKey(), base::WrapUnique(this));

  instances_.push_back(this);

  // Set WebPreferences defaults onto the JS object
  SetDefaultBoolIfUndefined(options::kPlugins, false);
  SetDefaultBoolIfUndefined(options::kExperimentalFeatures, false);
  SetDefaultBoolIfUndefined(options::kExperimentalCanvasFeatures, false);
  bool node = SetDefaultBoolIfUndefined(options::kNodeIntegration, true);
  SetDefaultBoolIfUndefined(options::kNodeIntegrationInWorker, false);
  SetDefaultBoolIfUndefined(options::kWebviewTag, node);
  SetDefaultBoolIfUndefined(options::kSandbox, false);
  SetDefaultBoolIfUndefined(options::kNativeWindowOpen, false);
  SetDefaultBoolIfUndefined(options::kContextIsolation, false);
  SetDefaultBoolIfUndefined("javascript", true);
  SetDefaultBoolIfUndefined("images", true);
  SetDefaultBoolIfUndefined("textAreasAreResizable", true);
  SetDefaultBoolIfUndefined("webgl", true);
  bool webSecurity = true;
  SetDefaultBoolIfUndefined(options::kWebSecurity, webSecurity);
  // If webSecurity was explicity set to false, let's inherit that into
  // insecureContent
  if (web_preferences.Get(options::kWebSecurity, &webSecurity) &&
      !webSecurity) {
    SetDefaultBoolIfUndefined(options::kAllowRunningInsecureContent, true);
  } else {
    SetDefaultBoolIfUndefined(options::kAllowRunningInsecureContent, false);
  }
#if defined(OS_MACOSX)
  SetDefaultBoolIfUndefined(options::kScrollBounce, false);
#endif
  SetDefaultBoolIfUndefined(options::kOffscreen, false);

  last_dict_ = std::move(*dict_.CreateDeepCopy());
}

WebContentsPreferences::~WebContentsPreferences() {
  instances_.erase(std::remove(instances_.begin(), instances_.end(), this),
                   instances_.end());
}

bool WebContentsPreferences::SetDefaultBoolIfUndefined(
    const base::StringPiece& key,
    bool val) {
  bool existing;
  if (!dict_.GetBoolean(key, &existing)) {
    dict_.SetBoolean(key, val);
    return val;
  }
  return existing;
}

bool WebContentsPreferences::IsEnabled(const base::StringPiece& name,
                                       bool default_value) {
  bool bool_value = default_value;
  dict_.GetBoolean(name, &bool_value);
  return bool_value;
}

void WebContentsPreferences::Merge(const base::DictionaryValue& extend) {
  dict_.MergeDictionary(&extend);
}

bool WebContentsPreferences::GetPreloadPath(
    base::FilePath::StringType* path) const {
  DCHECK(path);
  base::FilePath::StringType preload;
  if (dict_.GetString(options::kPreloadScript, &preload)) {
    if (base::FilePath(preload).IsAbsolute()) {
      *path = std::move(preload);
      return true;
    } else {
      LOG(ERROR) << "preload script must have absolute path.";
    }
  } else if (dict_.GetString(options::kPreloadURL, &preload)) {
    // Translate to file path if there is "preload-url" option.
    base::FilePath preload_path;
    if (net::FileURLToFilePath(GURL(preload), &preload_path)) {
      *path = std::move(preload_path.value());
      return true;
    } else {
      LOG(ERROR) << "preload url must be file:// protocol.";
    }
  }
  return false;
}

// static
content::WebContents* WebContentsPreferences::GetWebContentsFromProcessID(
    int process_id) {
  for (WebContentsPreferences* preferences : instances_) {
    content::WebContents* web_contents = preferences->web_contents_;
    if (web_contents->GetMainFrame()->GetProcess()->GetID() == process_id)
      return web_contents;
  }
  return nullptr;
}

// static
WebContentsPreferences* WebContentsPreferences::From(
    content::WebContents* web_contents) {
  if (!web_contents)
    return nullptr;
  return FromWebContents(web_contents);
}

void WebContentsPreferences::AppendCommandLineSwitches(
    base::CommandLine* command_line) {
  bool b;
  // Check if plugins are enabled.
  if (dict_.GetBoolean(options::kPlugins, &b) && b)
    command_line->AppendSwitch(switches::kEnablePlugins);

  // Experimental flags.
  if (dict_.GetBoolean(options::kExperimentalFeatures, &b) && b)
    command_line->AppendSwitch(
        ::switches::kEnableExperimentalWebPlatformFeatures);
  if (dict_.GetBoolean(options::kExperimentalCanvasFeatures, &b) && b)
    command_line->AppendSwitch(::switches::kEnableExperimentalCanvasFeatures);

  // Check if we have node integration specified.
  bool node_integration = true;
  dict_.GetBoolean(options::kNodeIntegration, &node_integration);
  command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                  node_integration ? "true" : "false");

  // Whether to enable node integration in Worker.
  if (dict_.GetBoolean(options::kNodeIntegrationInWorker, &b) && b)
    command_line->AppendSwitch(switches::kNodeIntegrationInWorker);

  // Check if webview tag creation is enabled, default to nodeIntegration value.
  // TODO(kevinsawicki): Default to false in 2.0
  bool webview_tag = node_integration;
  dict_.GetBoolean(options::kWebviewTag, &webview_tag);
  command_line->AppendSwitchASCII(switches::kWebviewTag,
                                  webview_tag ? "true" : "false");

  // If the `sandbox` option was passed to the BrowserWindow's webPreferences,
  // pass `--enable-sandbox` to the renderer so it won't have any node.js
  // integration.
  if (dict_.GetBoolean(options::kSandbox, &b) && b)
    command_line->AppendSwitch(switches::kEnableSandbox);
  else if (!command_line->HasSwitch(switches::kEnableSandbox))
    command_line->AppendSwitch(::switches::kNoSandbox);
  if (dict_.GetBoolean(options::kNativeWindowOpen, &b) && b)
    command_line->AppendSwitch(switches::kNativeWindowOpen);

  // The preload script.
  base::FilePath::StringType preload;
  if (GetPreloadPath(&preload))
    command_line->AppendSwitchNative(switches::kPreloadScript, preload);

  // Custom args for renderer process
  base::Value* customArgs;
  if (dict_.Get(options::kCustomArgs, &customArgs) && customArgs->is_list()) {
    for (const base::Value& customArg : customArgs->GetList()) {
      if (customArg.is_string())
        command_line->AppendArg(customArg.GetString());
    }
  }

  // Run Electron APIs and preload script in isolated world
  if (dict_.GetBoolean(options::kContextIsolation, &b) && b)
    command_line->AppendSwitch(switches::kContextIsolation);

  // --background-color.
  std::string s;
  if (dict_.GetString(options::kBackgroundColor, &s))
    command_line->AppendSwitchASCII(switches::kBackgroundColor, s);

  // --guest-instance-id, which is used to identify guest WebContents.
  int guest_instance_id = 0;
  if (dict_.GetInteger(options::kGuestInstanceID, &guest_instance_id))
    command_line->AppendSwitchASCII(switches::kGuestInstanceID,
                                    base::IntToString(guest_instance_id));

  // Pass the opener's window id.
  int opener_id;
  if (dict_.GetInteger(options::kOpenerID, &opener_id))
    command_line->AppendSwitchASCII(switches::kOpenerID,
                                    base::IntToString(opener_id));

#if defined(OS_MACOSX)
  // Enable scroll bounce.
  if (dict_.GetBoolean(options::kScrollBounce, &b) && b)
    command_line->AppendSwitch(switches::kScrollBounce);
#endif

  // Custom command line switches.
  const base::ListValue* args;
  if (dict_.GetList("commandLineSwitches", &args)) {
    for (size_t i = 0; i < args->GetSize(); ++i) {
      std::string arg;
      if (args->GetString(i, &arg) && !arg.empty())
        command_line->AppendSwitch(arg);
    }
  }

  // Enable blink features.
  if (dict_.GetString(options::kEnableBlinkFeatures, &s))
    command_line->AppendSwitchASCII(::switches::kEnableBlinkFeatures, s);

  // Disable blink features.
  if (dict_.GetString(options::kDisableBlinkFeatures, &s))
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures, s);

  if (guest_instance_id) {
    // Webview `document.visibilityState` tracks window visibility so we need
    // to let it know if the window happens to be hidden right now.
    auto* manager = WebViewManager::GetWebViewManager(web_contents_);
    if (manager) {
      auto* embedder = manager->GetEmbedder(guest_instance_id);
      if (embedder) {
        auto* relay = NativeWindowRelay::FromWebContents(embedder);
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

  // We are appending args to a webContents so let's save the current state
  // of our preferences object so that during the lifetime of the WebContents
  // we can fetch the options used to initally configure the WebContents
  last_dict_ = std::move(*dict_.CreateDeepCopy());
}

void WebContentsPreferences::OverrideWebkitPrefs(
    content::WebPreferences* prefs) {
  bool b;
  if (dict_.GetBoolean("javascript", &b))
    prefs->javascript_enabled = b;
  if (dict_.GetBoolean("images", &b))
    prefs->images_enabled = b;
  if (dict_.GetBoolean("textAreasAreResizable", &b))
    prefs->text_areas_are_resizable = b;
  if (dict_.GetBoolean("webgl", &b)) {
    prefs->webgl1_enabled = b;
    prefs->webgl2_enabled = b;
  }
  if (dict_.GetBoolean(options::kWebSecurity, &b)) {
    prefs->web_security_enabled = b;
    prefs->allow_running_insecure_content = !b;
  }
  if (dict_.GetBoolean(options::kAllowRunningInsecureContent, &b))
    prefs->allow_running_insecure_content = b;
  if (dict_.GetBoolean("navigateOnDragDrop", &b))
    prefs->navigate_on_drag_drop = b;
  const base::DictionaryValue* fonts = nullptr;
  if (dict_.GetDictionary("defaultFontFamily", &fonts)) {
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
  if (GetInteger("defaultFontSize", &size))
    prefs->default_font_size = size;
  if (GetInteger("defaultMonospaceFontSize", &size))
    prefs->default_fixed_font_size = size;
  if (GetInteger("minimumFontSize", &size))
    prefs->minimum_font_size = size;
  std::string encoding;
  if (dict_.GetString("defaultEncoding", &encoding))
    prefs->default_encoding = encoding;
}

bool WebContentsPreferences::GetInteger(const base::StringPiece& attribute_name,
                                        int* val) {
  // if it is already an integer, no conversion needed
  if (dict_.GetInteger(attribute_name, val))
    return true;

  std::string str;
  if (dict_.GetString(attribute_name, &str))
    return base::StringToInt(str, val);

  return false;
}

}  // namespace atom
