// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_contents_preferences.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/base/switches.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "net/base/filename_util.h"
#include "sandbox/policy/switches.h"
#include "shell/browser/native_window.h"
#include "shell/browser/web_view_manager.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/v8_cache_options.mojom.h"

#if defined(OS_WIN)
#include "ui/gfx/switches.h"
#endif

namespace {

bool GetAsString(const base::Value* val,
                 base::StringPiece path,
                 std::string* out) {
  if (val) {
    auto* found = val->FindKeyOfType(path, base::Value::Type::STRING);
    if (found) {
      *out = found->GetString();
      return true;
    }
  }
  return false;
}

bool GetAsString(const base::Value* val,
                 base::StringPiece path,
                 base::string16* out) {
  if (val) {
    auto* found = val->FindKeyOfType(path, base::Value::Type::STRING);
    if (found) {
      *out = base::UTF8ToUTF16(found->GetString());
      return true;
    }
  }
  return false;
}

bool GetAsInteger(const base::Value* val, base::StringPiece path, int* out) {
  if (val) {
    auto* found = val->FindKey(path);
    if (found && found->is_int()) {
      *out = found->GetInt();
      return true;
    } else if (found && found->is_string()) {
      return base::StringToInt(found->GetString(), out);
    }
  }
  return false;
}

bool GetAsAutoplayPolicy(const base::Value* val,
                         base::StringPiece path,
                         blink::mojom::AutoplayPolicy* out) {
  std::string policy_str;
  if (GetAsString(val, path, &policy_str)) {
    if (policy_str == "no-user-gesture-required") {
      *out = blink::mojom::AutoplayPolicy::kNoUserGestureRequired;
      return true;
    } else if (policy_str == "user-gesture-required") {
      *out = blink::mojom::AutoplayPolicy::kUserGestureRequired;
      return true;
    } else if (policy_str == "document-user-activation-required") {
      *out = blink::mojom::AutoplayPolicy::kDocumentUserActivationRequired;
      return true;
    }
    return false;
  }
  return false;
}

}  // namespace

namespace electron {

// static
std::vector<WebContentsPreferences*> WebContentsPreferences::instances_;

WebContentsPreferences::WebContentsPreferences(
    content::WebContents* web_contents,
    const gin_helper::Dictionary& web_preferences)
    : web_contents_(web_contents) {
  v8::Isolate* isolate = web_preferences.isolate();
  gin_helper::Dictionary copied(isolate, web_preferences.GetHandle()->Clone());
  // Following fields should not be stored.
  copied.Delete("embedder");
  copied.Delete("session");
  copied.Delete("type");

  gin::ConvertFromV8(isolate, copied.GetHandle(), &preference_);
  web_contents->SetUserData(UserDataKey(), base::WrapUnique(this));

  instances_.push_back(this);

  // Set WebPreferences defaults onto the JS object
  SetDefaultBoolIfUndefined(options::kPlugins, false);
  SetDefaultBoolIfUndefined(options::kExperimentalFeatures, false);
  SetDefaultBoolIfUndefined(options::kNodeIntegration, false);
  SetDefaultBoolIfUndefined(options::kNodeIntegrationInSubFrames, false);
  SetDefaultBoolIfUndefined(options::kNodeIntegrationInWorker, false);
  SetDefaultBoolIfUndefined(options::kDisableHtmlFullscreenWindowResize, false);
  SetDefaultBoolIfUndefined(options::kWebviewTag, false);
  SetDefaultBoolIfUndefined(options::kSandbox, false);
  SetDefaultBoolIfUndefined(options::kNativeWindowOpen, false);
  if (IsUndefined(options::kContextIsolation)) {
    node::Environment* env = node::Environment::GetCurrent(isolate);
    EmitWarning(env,
                "The default of contextIsolation is deprecated and will be "
                "changing from false to true in a future release of Electron.  "
                "See https://github.com/electron/electron/issues/23506 for "
                "more information",
                "electron");
  }
  SetDefaultBoolIfUndefined(options::kContextIsolation, false);
  SetDefaultBoolIfUndefined(options::kWorldSafeExecuteJavaScript, false);
  SetDefaultBoolIfUndefined(options::kJavaScript, true);
  SetDefaultBoolIfUndefined(options::kImages, true);
  SetDefaultBoolIfUndefined(options::kTextAreasAreResizable, true);
  SetDefaultBoolIfUndefined(options::kWebGL, true);
  SetDefaultBoolIfUndefined(options::kEnableWebSQL, true);
  SetDefaultBoolIfUndefined(options::kEnablePreferredSizeMode, false);
  bool webSecurity = true;
  SetDefaultBoolIfUndefined(options::kWebSecurity, webSecurity);
  // If webSecurity was explicitly set to false, let's inherit that into
  // insecureContent
  if (web_preferences.Get(options::kWebSecurity, &webSecurity) &&
      !webSecurity) {
    SetDefaultBoolIfUndefined(options::kAllowRunningInsecureContent, true);
  } else {
    SetDefaultBoolIfUndefined(options::kAllowRunningInsecureContent, false);
  }
#if defined(OS_MAC)
  SetDefaultBoolIfUndefined(options::kScrollBounce, false);
#endif
  SetDefaultBoolIfUndefined(options::kOffscreen, false);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SetDefaultBoolIfUndefined(options::kSpellcheck, true);
#endif

  // If this is a <webview> tag, and the embedder is offscreen-rendered, then
  // this WebContents is also offscreen-rendered.
  int guest_instance_id = 0;
  if (web_preferences.Get(options::kGuestInstanceID, &guest_instance_id)) {
    auto* manager = WebViewManager::GetWebViewManager(web_contents);
    if (manager) {
      auto* embedder = manager->GetEmbedder(guest_instance_id);
      if (embedder) {
        auto* embedder_preferences = WebContentsPreferences::From(embedder);
        if (embedder_preferences &&
            embedder_preferences->IsEnabled(options::kOffscreen)) {
          preference_.SetKey(options::kOffscreen, base::Value(true));
        }
      }
    }
  }

  SetDefaults();
}

WebContentsPreferences::~WebContentsPreferences() {
  instances_.erase(std::remove(instances_.begin(), instances_.end(), this),
                   instances_.end());
}

void WebContentsPreferences::SetDefaults() {
  if (IsEnabled(options::kSandbox)) {
    SetBool(options::kNativeWindowOpen, true);
  }

  last_preference_ = preference_.Clone();
}

bool WebContentsPreferences::IsUndefined(base::StringPiece key) {
  return !preference_.FindKeyOfType(key, base::Value::Type::BOOLEAN);
}

bool WebContentsPreferences::SetDefaultBoolIfUndefined(base::StringPiece key,
                                                       bool val) {
  auto* current_value =
      preference_.FindKeyOfType(key, base::Value::Type::BOOLEAN);
  if (current_value) {
    return current_value->GetBool();
  } else {
    preference_.SetKey(key, base::Value(val));
    return val;
  }
}

void WebContentsPreferences::SetBool(base::StringPiece key, bool value) {
  preference_.SetKey(key, base::Value(value));
}

bool WebContentsPreferences::IsEnabled(base::StringPiece name,
                                       bool default_value) const {
  auto* current_value =
      preference_.FindKeyOfType(name, base::Value::Type::BOOLEAN);
  if (current_value)
    return current_value->GetBool();
  return default_value;
}

void WebContentsPreferences::Merge(const base::DictionaryValue& extend) {
  if (preference_.is_dict())
    static_cast<base::DictionaryValue*>(&preference_)->MergeDictionary(&extend);

  SetDefaults();
}

void WebContentsPreferences::Clear() {
  if (preference_.is_dict())
    static_cast<base::DictionaryValue*>(&preference_)->Clear();
}

bool WebContentsPreferences::GetPreference(base::StringPiece name,
                                           std::string* value) const {
  return GetAsString(&preference_, name, value);
}

bool WebContentsPreferences::GetPreloadPath(base::FilePath* path) const {
  DCHECK(path);
  base::FilePath::StringType preload_path;
  if (GetAsString(&preference_, options::kPreloadScript, &preload_path)) {
    base::FilePath preload(preload_path);
    if (preload.IsAbsolute()) {
      *path = std::move(preload);
      return true;
    } else {
      LOG(ERROR) << "preload script must have absolute path.";
    }
  } else if (GetAsString(&preference_, options::kPreloadURL, &preload_path)) {
    // Translate to file path if there is "preload-url" option.
    base::FilePath preload;
    if (net::FileURLToFilePath(GURL(preload_path), &preload)) {
      *path = std::move(preload);
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
    base::CommandLine* command_line,
    bool is_subframe) {
  // Experimental flags.
  if (IsEnabled(options::kExperimentalFeatures))
    command_line->AppendSwitch(
        ::switches::kEnableExperimentalWebPlatformFeatures);

  // Sandbox can be enabled for renderer processes hosting cross-origin frames
  // unless nodeIntegrationInSubFrames is enabled
  bool can_sandbox_frame =
      is_subframe && !IsEnabled(options::kNodeIntegrationInSubFrames);

  if (IsEnabled(options::kSandbox) || can_sandbox_frame) {
    command_line->AppendSwitch(switches::kEnableSandbox);
  } else if (!command_line->HasSwitch(switches::kEnableSandbox)) {
    command_line->AppendSwitch(sandbox::policy::switches::kNoSandbox);
    command_line->AppendSwitch(::switches::kNoZygote);
  }

  // Custom args for renderer process
  auto* customArgs =
      preference_.FindKeyOfType(options::kCustomArgs, base::Value::Type::LIST);
  if (customArgs) {
    for (const auto& customArg : customArgs->GetList()) {
      if (customArg.is_string())
        command_line->AppendArg(customArg.GetString());
    }
  }

  // --offscreen
  // TODO(loc): Offscreen is duplicated in WebPreferences because it's needed
  // earlier than we can get WebPreferences at the moment.
  if (IsEnabled(options::kOffscreen)) {
    command_line->AppendSwitch(options::kOffscreen);
  }

#if defined(OS_MAC)
  // Enable scroll bounce.
  if (IsEnabled(options::kScrollBounce))
    command_line->AppendSwitch(switches::kScrollBounce);
#endif

  // Custom command line switches.
  auto* args =
      preference_.FindKeyOfType("commandLineSwitches", base::Value::Type::LIST);
  if (args) {
    for (const auto& arg : args->GetList()) {
      if (arg.is_string()) {
        const auto& arg_val = arg.GetString();
        if (!arg_val.empty())
          command_line->AppendSwitch(arg_val);
      }
    }
  }

  std::string s;
  // Enable blink features.
  if (GetAsString(&preference_, options::kEnableBlinkFeatures, &s))
    command_line->AppendSwitchASCII(::switches::kEnableBlinkFeatures, s);

  // Disable blink features.
  if (GetAsString(&preference_, options::kDisableBlinkFeatures, &s))
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures, s);

  if (IsEnabled(options::kNodeIntegrationInWorker))
    command_line->AppendSwitch(switches::kNodeIntegrationInWorker);

  // We are appending args to a webContents so let's save the current state
  // of our preferences object so that during the lifetime of the WebContents
  // we can fetch the options used to initally configure the WebContents
  last_preference_ = preference_.Clone();
}

void WebContentsPreferences::OverrideWebkitPrefs(
    blink::web_pref::WebPreferences* prefs) {
  prefs->javascript_enabled =
      IsEnabled(options::kJavaScript, true /* default_value */);
  prefs->images_enabled = IsEnabled(options::kImages, true /* default_value */);
  prefs->text_areas_are_resizable =
      IsEnabled(options::kTextAreasAreResizable, true /* default_value */);
  prefs->navigate_on_drag_drop =
      IsEnabled(options::kNavigateOnDragDrop, false /* default_value */);
  if (!GetAsAutoplayPolicy(&preference_, "autoplayPolicy",
                           &prefs->autoplay_policy)) {
    prefs->autoplay_policy =
        blink::mojom::AutoplayPolicy::kNoUserGestureRequired;
  }

  // Check if webgl should be enabled.
  bool is_webgl_enabled = IsEnabled(options::kWebGL, true /* default_value */);
  prefs->webgl1_enabled = is_webgl_enabled;
  prefs->webgl2_enabled = is_webgl_enabled;

  // Check if web security should be enabled.
  bool is_web_security_enabled =
      IsEnabled(options::kWebSecurity, true /* default_value */);
  prefs->web_security_enabled = is_web_security_enabled;
  prefs->allow_running_insecure_content =
      IsEnabled(options::kAllowRunningInsecureContent,
                !is_web_security_enabled /* default_value */);

  auto* fonts_dict = preference_.FindKeyOfType("defaultFontFamily",
                                               base::Value::Type::DICTIONARY);
  if (fonts_dict) {
    base::string16 font;
    if (GetAsString(fonts_dict, "standard", &font))
      prefs->standard_font_family_map[blink::web_pref::kCommonScript] = font;
    if (GetAsString(fonts_dict, "serif", &font))
      prefs->serif_font_family_map[blink::web_pref::kCommonScript] = font;
    if (GetAsString(fonts_dict, "sansSerif", &font))
      prefs->sans_serif_font_family_map[blink::web_pref::kCommonScript] = font;
    if (GetAsString(fonts_dict, "monospace", &font))
      prefs->fixed_font_family_map[blink::web_pref::kCommonScript] = font;
    if (GetAsString(fonts_dict, "cursive", &font))
      prefs->cursive_font_family_map[blink::web_pref::kCommonScript] = font;
    if (GetAsString(fonts_dict, "fantasy", &font))
      prefs->fantasy_font_family_map[blink::web_pref::kCommonScript] = font;
  }

  int size;
  if (GetAsInteger(&preference_, "defaultFontSize", &size))
    prefs->default_font_size = size;
  if (GetAsInteger(&preference_, "defaultMonospaceFontSize", &size))
    prefs->default_fixed_font_size = size;
  if (GetAsInteger(&preference_, "minimumFontSize", &size))
    prefs->minimum_font_size = size;
  std::string encoding;
  if (GetAsString(&preference_, "defaultEncoding", &encoding))
    prefs->default_encoding = encoding;

  // --background-color.
  std::string color;
  if (GetAsString(&preference_, options::kBackgroundColor, &color)) {
    prefs->background_color = color;
  } else if (!IsEnabled(options::kOffscreen)) {
    prefs->background_color = "#fff";
  }

  // Pass the opener's window id.
  int opener_id;
  if (GetAsInteger(&preference_, options::kOpenerID, &opener_id))
    prefs->opener_id = opener_id;

  // Run Electron APIs and preload script in isolated world
  prefs->context_isolation = IsEnabled(options::kContextIsolation);

#if BUILDFLAG(ENABLE_REMOTE_MODULE)
  // Whether to enable the remote module
  prefs->enable_remote_module = IsEnabled(options::kEnableRemoteModule, false);
#endif

  prefs->world_safe_execute_javascript =
      IsEnabled(options::kWorldSafeExecuteJavaScript);

  int guest_instance_id = 0;
  if (GetAsInteger(&preference_, options::kGuestInstanceID, &guest_instance_id))
    prefs->guest_instance_id = guest_instance_id;

  prefs->hidden_page = false;
  if (guest_instance_id) {
    // Webview `document.visibilityState` tracks window visibility so we need
    // to let it know if the window happens to be hidden right now.
    auto* manager = WebViewManager::GetWebViewManager(web_contents_);
    if (manager) {
      auto* embedder = manager->GetEmbedder(guest_instance_id);
      if (embedder) {
        auto* relay = NativeWindowRelay::FromWebContents(embedder);
        if (relay) {
          auto* window = relay->GetNativeWindow();
          if (window) {
            const bool visible = window->IsVisible() && !window->IsMinimized();
            if (!visible) {
              prefs->hidden_page = true;
            }
          }
        }
      }
    }
  }

  prefs->offscreen = IsEnabled(options::kOffscreen);

  // The preload script.
  GetPreloadPath(&prefs->preload);

  // Check if nativeWindowOpen is enabled.
  prefs->native_window_open = IsEnabled(options::kNativeWindowOpen);

  // Check if we have node integration specified.
  prefs->node_integration = IsEnabled(options::kNodeIntegration);

  // Whether to enable node integration in Worker.
  prefs->node_integration_in_worker =
      IsEnabled(options::kNodeIntegrationInWorker);

  prefs->node_integration_in_sub_frames =
      IsEnabled(options::kNodeIntegrationInSubFrames);

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  prefs->enable_spellcheck = IsEnabled(options::kSpellcheck);
#endif

  // Check if plugins are enabled.
  prefs->enable_plugins = IsEnabled(options::kPlugins);

  // Check if webview tag creation is enabled, default to nodeIntegration value.
  prefs->webview_tag = IsEnabled(options::kWebviewTag);

  // Whether to allow the WebSQL api
  prefs->enable_websql = IsEnabled(options::kEnableWebSQL);

  std::string v8_cache_options;
  if (GetAsString(&preference_, "v8CacheOptions", &v8_cache_options)) {
    if (v8_cache_options == "none") {
      prefs->v8_cache_options = blink::mojom::V8CacheOptions::kNone;
    } else if (v8_cache_options == "code") {
      prefs->v8_cache_options = blink::mojom::V8CacheOptions::kCode;
    } else if (v8_cache_options == "bypassHeatCheck") {
      prefs->v8_cache_options =
          blink::mojom::V8CacheOptions::kCodeWithoutHeatCheck;
    } else if (v8_cache_options == "bypassHeatCheckAndEagerCompile") {
      prefs->v8_cache_options =
          blink::mojom::V8CacheOptions::kFullCodeWithoutHeatCheck;
    } else {
      prefs->v8_cache_options = blink::mojom::V8CacheOptions::kDefault;
    }
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsPreferences)

}  // namespace electron
