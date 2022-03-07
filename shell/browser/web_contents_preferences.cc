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
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/content_switches.h"
#include "net/base/filename_util.h"
#include "sandbox/policy/switches.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/native_window.h"
#include "shell/browser/session_preferences.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/v8_cache_options.mojom.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom.h"

#if BUILDFLAG(IS_WIN)
#include "ui/gfx/switches.h"
#endif

namespace gin {

template <>
struct Converter<blink::mojom::AutoplayPolicy> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::AutoplayPolicy* out) {
    std::string policy_str;
    if (!ConvertFromV8(isolate, val, &policy_str))
      return false;
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
};

template <>
struct Converter<blink::mojom::V8CacheOptions> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::V8CacheOptions* out) {
    std::string v8_cache_options;
    if (!ConvertFromV8(isolate, val, &v8_cache_options))
      return false;
    if (v8_cache_options == "none") {
      *out = blink::mojom::V8CacheOptions::kNone;
      return true;
    } else if (v8_cache_options == "code") {
      *out = blink::mojom::V8CacheOptions::kCode;
      return true;
    } else if (v8_cache_options == "bypassHeatCheck") {
      *out = blink::mojom::V8CacheOptions::kCodeWithoutHeatCheck;
      return true;
    } else if (v8_cache_options == "bypassHeatCheckAndEagerCompile") {
      *out = blink::mojom::V8CacheOptions::kFullCodeWithoutHeatCheck;
      return true;
    }
    return false;
  }
};

}  // namespace gin

namespace electron {

namespace {
std::vector<WebContentsPreferences*>& Instances() {
  static base::NoDestructor<std::vector<WebContentsPreferences*>> g_instances;
  return *g_instances;
}
}  // namespace

WebContentsPreferences::WebContentsPreferences(
    content::WebContents* web_contents,
    const gin_helper::Dictionary& web_preferences)
    : content::WebContentsUserData<WebContentsPreferences>(*web_contents),
      web_contents_(web_contents) {
  web_contents->SetUserData(UserDataKey(), base::WrapUnique(this));
  Instances().push_back(this);
  SetFromDictionary(web_preferences);

  // If this is a <webview> tag, and the embedder is offscreen-rendered, then
  // this WebContents is also offscreen-rendered.
  if (auto* api_web_contents = api::WebContents::From(web_contents_)) {
    if (electron::api::WebContents* embedder = api_web_contents->embedder()) {
      auto* embedder_preferences =
          WebContentsPreferences::From(embedder->web_contents());
      if (embedder_preferences && embedder_preferences->IsOffscreen()) {
        offscreen_ = true;
      }
    }
  }
}

WebContentsPreferences::~WebContentsPreferences() {
  Instances().erase(std::remove(Instances().begin(), Instances().end(), this),
                    Instances().end());
}

void WebContentsPreferences::Clear() {
  plugins_ = false;
  experimental_features_ = false;
  node_integration_ = false;
  node_integration_in_sub_frames_ = false;
  node_integration_in_worker_ = false;
  disable_html_fullscreen_window_resize_ = false;
  webview_tag_ = false;
  sandbox_ = absl::nullopt;
  context_isolation_ = true;
  javascript_ = true;
  images_ = true;
  text_areas_are_resizable_ = true;
  webgl_ = true;
  enable_websql_ = true;
  enable_preferred_size_mode_ = false;
  web_security_ = true;
  allow_running_insecure_content_ = false;
  offscreen_ = false;
  navigate_on_drag_drop_ = false;
  autoplay_policy_ = blink::mojom::AutoplayPolicy::kNoUserGestureRequired;
  default_font_family_.clear();
  default_font_size_ = absl::nullopt;
  default_monospace_font_size_ = absl::nullopt;
  minimum_font_size_ = absl::nullopt;
  default_encoding_ = absl::nullopt;
  is_webview_ = false;
  custom_args_.clear();
  custom_switches_.clear();
  enable_blink_features_ = absl::nullopt;
  disable_blink_features_ = absl::nullopt;
  disable_popups_ = false;
  disable_dialogs_ = false;
  safe_dialogs_ = false;
  safe_dialogs_message_ = absl::nullopt;
  ignore_menu_shortcuts_ = false;
  background_color_ = absl::nullopt;
  image_animation_policy_ =
      blink::mojom::ImageAnimationPolicy::kImageAnimationPolicyAllowed;
  preload_path_ = absl::nullopt;
  v8_cache_options_ = blink::mojom::V8CacheOptions::kDefault;

#if BUILDFLAG(IS_MAC)
  scroll_bounce_ = false;
#endif
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  spellcheck_ = true;
#endif
}

void WebContentsPreferences::SetFromDictionary(
    const gin_helper::Dictionary& web_preferences) {
  Clear();
  Merge(web_preferences);
}

void WebContentsPreferences::Merge(
    const gin_helper::Dictionary& web_preferences) {
  web_preferences.Get(options::kPlugins, &plugins_);
  web_preferences.Get(options::kExperimentalFeatures, &experimental_features_);
  web_preferences.Get(options::kNodeIntegration, &node_integration_);
  web_preferences.Get(options::kNodeIntegrationInSubFrames,
                      &node_integration_in_sub_frames_);
  web_preferences.Get(options::kNodeIntegrationInWorker,
                      &node_integration_in_worker_);
  web_preferences.Get(options::kDisableHtmlFullscreenWindowResize,
                      &disable_html_fullscreen_window_resize_);
  web_preferences.Get(options::kWebviewTag, &webview_tag_);
  bool sandbox;
  if (web_preferences.Get(options::kSandbox, &sandbox))
    sandbox_ = sandbox;
  web_preferences.Get(options::kContextIsolation, &context_isolation_);
  web_preferences.Get(options::kJavaScript, &javascript_);
  web_preferences.Get(options::kImages, &images_);
  web_preferences.Get(options::kTextAreasAreResizable,
                      &text_areas_are_resizable_);
  web_preferences.Get(options::kWebGL, &webgl_);
  web_preferences.Get(options::kEnableWebSQL, &enable_websql_);
  web_preferences.Get(options::kEnablePreferredSizeMode,
                      &enable_preferred_size_mode_);
  web_preferences.Get(options::kWebSecurity, &web_security_);
  if (!web_preferences.Get(options::kAllowRunningInsecureContent,
                           &allow_running_insecure_content_) &&
      !web_security_)
    allow_running_insecure_content_ = true;
  web_preferences.Get(options::kOffscreen, &offscreen_);
  web_preferences.Get(options::kNavigateOnDragDrop, &navigate_on_drag_drop_);
  web_preferences.Get("autoplayPolicy", &autoplay_policy_);
  web_preferences.Get("defaultFontFamily", &default_font_family_);
  int size;
  if (web_preferences.Get("defaultFontSize", &size))
    default_font_size_ = size;
  if (web_preferences.Get("defaultMonospaceFontSize", &size))
    default_monospace_font_size_ = size;
  if (web_preferences.Get("minimumFontSize", &size))
    minimum_font_size_ = size;
  std::string encoding;
  if (web_preferences.Get("defaultEncoding", &encoding))
    default_encoding_ = encoding;
  web_preferences.Get(options::kCustomArgs, &custom_args_);
  web_preferences.Get("commandLineSwitches", &custom_switches_);
  web_preferences.Get("disablePopups", &disable_popups_);
  web_preferences.Get("disableDialogs", &disable_dialogs_);
  web_preferences.Get("safeDialogs", &safe_dialogs_);
  // preferences don't save a transparency option,
  // apply any existing transparency setting to background_color_
  bool transparent;
  if (web_preferences.Get(options::kTransparent, &transparent)) {
    background_color_ = SK_ColorTRANSPARENT;
  }
  std::string background_color;
  if (web_preferences.GetHidden(options::kBackgroundColor, &background_color))
    background_color_ = ParseHexColor(background_color);
  std::string safe_dialogs_message;
  if (web_preferences.Get("safeDialogsMessage", &safe_dialogs_message))
    safe_dialogs_message_ = safe_dialogs_message;
  web_preferences.Get("ignoreMenuShortcuts", &ignore_menu_shortcuts_);
  std::string enable_blink_features;
  if (web_preferences.Get(options::kEnableBlinkFeatures,
                          &enable_blink_features))
    enable_blink_features_ = enable_blink_features;
  std::string disable_blink_features;
  if (web_preferences.Get(options::kDisableBlinkFeatures,
                          &disable_blink_features))
    disable_blink_features_ = disable_blink_features;

  base::FilePath::StringType preload_path;
  std::string preload_url_str;
  if (web_preferences.Get(options::kPreloadScript, &preload_path)) {
    base::FilePath preload(preload_path);
    if (preload.IsAbsolute()) {
      preload_path_ = preload;
    } else {
      LOG(ERROR) << "preload script must have absolute path.";
    }
  } else if (web_preferences.Get(options::kPreloadURL, &preload_url_str)) {
    // Translate to file path if there is "preload-url" option.
    base::FilePath preload;
    GURL preload_url(preload_url_str);
    if (net::FileURLToFilePath(preload_url, &preload)) {
      preload_path_ = preload;
    } else {
      LOG(ERROR) << "preload url must be file:// protocol.";
    }
  }

  std::string type;
  if (web_preferences.Get(options::kType, &type)) {
    is_webview_ = type == "webview";
  }

  web_preferences.Get("v8CacheOptions", &v8_cache_options_);

#if BUILDFLAG(IS_MAC)
  web_preferences.Get(options::kScrollBounce, &scroll_bounce_);
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  web_preferences.Get(options::kSpellcheck, &spellcheck_);
#endif

  SaveLastPreferences();
}

bool WebContentsPreferences::GetSafeDialogsMessage(std::string* message) const {
  if (safe_dialogs_message_) {
    *message = *safe_dialogs_message_;
    return true;
  }
  return false;
}

bool WebContentsPreferences::SetImageAnimationPolicy(std::string policy) {
  if (policy == "animate") {
    image_animation_policy_ =
        blink::mojom::ImageAnimationPolicy::kImageAnimationPolicyAllowed;
    return true;
  } else if (policy == "animateOnce") {
    image_animation_policy_ =
        blink::mojom::ImageAnimationPolicy::kImageAnimationPolicyAnimateOnce;
    return true;
  } else if (policy == "noAnimation") {
    image_animation_policy_ =
        blink::mojom::ImageAnimationPolicy::kImageAnimationPolicyNoAnimation;
    return true;
  }
  return false;
}

bool WebContentsPreferences::GetPreloadPath(base::FilePath* path) const {
  DCHECK(path);
  if (preload_path_) {
    *path = *preload_path_;
    return true;
  }
  return false;
}

bool WebContentsPreferences::IsSandboxed() const {
  if (sandbox_)
    return *sandbox_;
  bool sandbox_disabled_by_default =
      node_integration_ || node_integration_in_worker_ || preload_path_ ||
      !SessionPreferences::GetValidPreloads(web_contents_->GetBrowserContext())
           .empty();
  return !sandbox_disabled_by_default;
}

// static
content::WebContents* WebContentsPreferences::GetWebContentsFromProcessID(
    int process_id) {
  for (WebContentsPreferences* preferences : Instances()) {
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
  if (experimental_features_)
    command_line->AppendSwitch(
        ::switches::kEnableExperimentalWebPlatformFeatures);

  // Sandbox can be enabled for renderer processes hosting cross-origin frames
  // unless nodeIntegrationInSubFrames is enabled
  bool can_sandbox_frame = is_subframe && !node_integration_in_sub_frames_;

  if (IsSandboxed() || can_sandbox_frame) {
    command_line->AppendSwitch(switches::kEnableSandbox);
  } else if (!command_line->HasSwitch(switches::kEnableSandbox)) {
    command_line->AppendSwitch(sandbox::policy::switches::kNoSandbox);
    command_line->AppendSwitch(::switches::kNoZygote);
  }

#if BUILDFLAG(IS_MAC)
  // Enable scroll bounce.
  if (scroll_bounce_)
    command_line->AppendSwitch(switches::kScrollBounce);
#endif

  // Custom args for renderer process
  for (const auto& arg : custom_args_)
    if (!arg.empty())
      command_line->AppendArg(arg);

  // Custom command line switches.
  for (const auto& arg : custom_switches_)
    if (!arg.empty())
      command_line->AppendSwitch(arg);

  if (enable_blink_features_)
    command_line->AppendSwitchASCII(::switches::kEnableBlinkFeatures,
                                    *enable_blink_features_);
  if (disable_blink_features_)
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures,
                                    *disable_blink_features_);

  if (node_integration_in_worker_)
    command_line->AppendSwitch(switches::kNodeIntegrationInWorker);

  // We are appending args to a webContents so let's save the current state
  // of our preferences object so that during the lifetime of the WebContents
  // we can fetch the options used to initally configure the WebContents
  // last_preference_ = preference_.Clone();
  SaveLastPreferences();
}

void WebContentsPreferences::SaveLastPreferences() {
  last_web_preferences_ = base::Value(base::Value::Type::DICTIONARY);
  last_web_preferences_.SetKey(options::kNodeIntegration,
                               base::Value(node_integration_));
  last_web_preferences_.SetKey(options::kNodeIntegrationInSubFrames,
                               base::Value(node_integration_in_sub_frames_));
  last_web_preferences_.SetKey(options::kSandbox, base::Value(IsSandboxed()));
  last_web_preferences_.SetKey(options::kContextIsolation,
                               base::Value(context_isolation_));
  last_web_preferences_.SetKey(options::kJavaScript, base::Value(javascript_));
  last_web_preferences_.SetKey(options::kEnableWebSQL,
                               base::Value(enable_websql_));
  last_web_preferences_.SetKey(options::kWebviewTag, base::Value(webview_tag_));
  last_web_preferences_.SetKey("disablePopups", base::Value(disable_popups_));
  last_web_preferences_.SetKey(options::kWebSecurity,
                               base::Value(web_security_));
  last_web_preferences_.SetKey(options::kAllowRunningInsecureContent,
                               base::Value(allow_running_insecure_content_));
  last_web_preferences_.SetKey(options::kExperimentalFeatures,
                               base::Value(experimental_features_));
  last_web_preferences_.SetKey(
      options::kEnableBlinkFeatures,
      base::Value(enable_blink_features_.value_or("")));
}

void WebContentsPreferences::OverrideWebkitPrefs(
    blink::web_pref::WebPreferences* prefs) {
  prefs->javascript_enabled = javascript_;
  prefs->images_enabled = images_;
  prefs->animation_policy = image_animation_policy_;
  prefs->text_areas_are_resizable = text_areas_are_resizable_;
  prefs->navigate_on_drag_drop = navigate_on_drag_drop_;
  prefs->autoplay_policy = autoplay_policy_;

  // Check if webgl should be enabled.
  prefs->webgl1_enabled = webgl_;
  prefs->webgl2_enabled = webgl_;

  // Check if web security should be enabled.
  prefs->web_security_enabled = web_security_;
  prefs->allow_running_insecure_content = allow_running_insecure_content_;

  if (auto font =
          default_font_family_.find("standard") != default_font_family_.end())
    prefs->standard_font_family_map[blink::web_pref::kCommonScript] = font;
  if (auto font =
          default_font_family_.find("serif") != default_font_family_.end())
    prefs->serif_font_family_map[blink::web_pref::kCommonScript] = font;
  if (auto font =
          default_font_family_.find("sansSerif") != default_font_family_.end())
    prefs->sans_serif_font_family_map[blink::web_pref::kCommonScript] = font;
  if (auto font =
          default_font_family_.find("monospace") != default_font_family_.end())
    prefs->fixed_font_family_map[blink::web_pref::kCommonScript] = font;
  if (auto font =
          default_font_family_.find("cursive") != default_font_family_.end())
    prefs->cursive_font_family_map[blink::web_pref::kCommonScript] = font;
  if (auto font =
          default_font_family_.find("fantasy") != default_font_family_.end())
    prefs->fantasy_font_family_map[blink::web_pref::kCommonScript] = font;

  if (default_font_size_)
    prefs->default_font_size = *default_font_size_;
  if (default_monospace_font_size_)
    prefs->default_fixed_font_size = *default_monospace_font_size_;
  if (minimum_font_size_)
    prefs->minimum_font_size = *minimum_font_size_;
  if (default_encoding_)
    prefs->default_encoding = *default_encoding_;

  // Run Electron APIs and preload script in isolated world
  prefs->context_isolation = context_isolation_;
  prefs->is_webview = is_webview_;

  prefs->hidden_page = false;
  // Webview `document.visibilityState` tracks window visibility so we need
  // to let it know if the window happens to be hidden right now.
  if (auto* api_web_contents = api::WebContents::From(web_contents_)) {
    if (electron::api::WebContents* embedder = api_web_contents->embedder()) {
      if (auto* relay =
              NativeWindowRelay::FromWebContents(embedder->web_contents())) {
        if (auto* window = relay->GetNativeWindow()) {
          const bool visible = window->IsVisible() && !window->IsMinimized();
          if (!visible) {
            prefs->hidden_page = true;
          }
        }
      }
    }
  }

  prefs->offscreen = offscreen_;

  // The preload script.
  if (preload_path_)
    prefs->preload = *preload_path_;

  prefs->node_integration = node_integration_;
  prefs->node_integration_in_worker = node_integration_in_worker_;
  prefs->node_integration_in_sub_frames = node_integration_in_sub_frames_;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  prefs->enable_spellcheck = spellcheck_;
#endif

  prefs->enable_plugins = plugins_;
  prefs->webview_tag = webview_tag_;
  prefs->enable_websql = enable_websql_;

  prefs->v8_cache_options = v8_cache_options_;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsPreferences);

}  // namespace electron
