// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_preferences.h"

#include <string>

#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/common/web_preferences.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"

#if defined(OS_WIN)
#include "ui/gfx/switches.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::WebContentsPreferences);

namespace atom {

namespace {

// Array of available web runtime features.
struct FeaturePair {
  const char* name;
  const char* cmd;
};
FeaturePair kWebRuntimeFeatures[] = {
  { options::kExperimentalFeatures,
    switches::kExperimentalFeatures },
  { options::kExperimentalCanvasFeatures,
    switches::kExperimentalCanvasFeatures },
  { options::kOverlayScrollbars,
    switches::kOverlayScrollbars },
  { options::kSharedWorker,
    switches::kSharedWorker },
  { options::kPageVisibility,
    switches::kPageVisibility },
};

}  // namespace

WebContentsPreferences::WebContentsPreferences(
    content::WebContents* web_contents,
    const mate::Dictionary& web_preferences) {
  v8::Isolate* isolate = web_preferences.isolate();
  mate::Dictionary copied(isolate, web_preferences.GetHandle()->Clone());
  // Following fields should not be stored.
  copied.Delete("embedder");
  copied.Delete("isGuest");
  copied.Delete("session");

  mate::ConvertFromV8(isolate, copied.GetHandle(), &web_preferences_);
  web_contents->SetUserData(UserDataKey(), this);
}

WebContentsPreferences::~WebContentsPreferences() {
}

void WebContentsPreferences::Merge(const base::DictionaryValue& extend) {
  web_preferences_.MergeDictionary(&extend);
}

// static
void WebContentsPreferences::AppendExtraCommandLineSwitches(
    content::WebContents* web_contents, base::CommandLine* command_line) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  base::DictionaryValue& web_preferences = self->web_preferences_;

  bool b;
#if defined(OS_WIN)
  // Check if DirectWrite is disabled.
  if (web_preferences.GetBoolean(options::kDirectWrite, &b) && !b)
    command_line->AppendSwitch(::switches::kDisableDirectWrite);
#endif

  // Check if plugins are enabled.
  if (web_preferences.GetBoolean("plugins", &b) && b)
    command_line->AppendSwitch(switches::kEnablePlugins);

  // This set of options are not availabe in WebPreferences, so we have to pass
  // them via command line and enable them in renderer procss.
  for (size_t i = 0; i < arraysize(kWebRuntimeFeatures); ++i) {
    const auto& feature = kWebRuntimeFeatures[i];
    if (web_preferences.GetBoolean(feature.name, &b))
      command_line->AppendSwitchASCII(feature.cmd, b ? "true" : "false");
  }

  // Check if we have node integration specified.
  bool node_integration = true;
  web_preferences.GetBoolean(options::kNodeIntegration, &node_integration);
  // Be compatible with old API of "node-integration" option.
  std::string old_token;
  if (web_preferences.GetString(options::kNodeIntegration, &old_token) &&
      old_token != "disable")
    node_integration = true;
  command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                  node_integration ? "true" : "false");

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

  // The zoom factor.
  double zoom_factor = 1.0;
  if (web_preferences.GetDouble(options::kZoomFactor, &zoom_factor) &&
      zoom_factor != 1.0)
    command_line->AppendSwitchASCII(switches::kZoomFactor,
                                    base::DoubleToString(zoom_factor));

  // --guest-instance-id, which is used to identify guest WebContents.
  int guest_instance_id;
  if (web_preferences.GetInteger(options::kGuestInstanceID, &guest_instance_id))
      command_line->AppendSwitchASCII(switches::kGuestInstanceID,
                                      base::IntToString(guest_instance_id));

  // Pass the opener's window id.
  int opener_id;
  if (web_preferences.GetInteger(options::kOpenerID, &opener_id))
      command_line->AppendSwitchASCII(switches::kOpenerID,
                                      base::IntToString(opener_id));
}

// static
void WebContentsPreferences::OverrideWebkitPrefs(
    content::WebContents* web_contents, content::WebPreferences* prefs) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  bool b;
  if (self->web_preferences_.GetBoolean("javascript", &b))
    prefs->javascript_enabled = b;
  if (self->web_preferences_.GetBoolean("images", &b))
    prefs->images_enabled = b;
  if (self->web_preferences_.GetBoolean("textAreasAreResizable", &b))
    prefs->text_areas_are_resizable = b;
  if (self->web_preferences_.GetBoolean("webgl", &b))
    prefs->experimental_webgl_enabled = b;
  if (self->web_preferences_.GetBoolean("webaudio", &b))
    prefs->webaudio_enabled = b;
  if (self->web_preferences_.GetBoolean("webSecurity", &b)) {
    prefs->web_security_enabled = b;
    prefs->allow_displaying_insecure_content = !b;
    prefs->allow_running_insecure_content = !b;
  }
  if (self->web_preferences_.GetBoolean("allowDisplayingInsecureContent", &b))
    prefs->allow_displaying_insecure_content = b;
  if (self->web_preferences_.GetBoolean("allowRunningInsecureContent", &b))
    prefs->allow_running_insecure_content = b;
}

}  // namespace atom
