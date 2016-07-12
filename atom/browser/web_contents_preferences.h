// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
#define ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_

#include <vector>

#include "atom/browser/api/atom_api_extension.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/content_switches.h"

#if defined(ENABLE_EXTENSIONS)
#include "extensions/common/switches.h"
#endif

namespace base {
class CommandLine;
}

namespace content {
struct WebPreferences;
}

namespace mate {
class Dictionary;
}

namespace atom {

// Stores and applies the preferences of WebContents.
class WebContentsPreferences
    : public content::WebContentsUserData<WebContentsPreferences> {
 public:
  // Get WebContents according to process ID.
  // FIXME(zcbenz): This method does not belong here.
  static content::WebContents* GetWebContentsFromProcessID(int process_id);

  // Append command paramters according to |web_contents|'s preferences.
  static void AppendExtraCommandLineSwitches(
      content::WebContents* web_contents, base::CommandLine* command_line);

  static bool run_node(const base::CommandLine* cmd_line) {
    // don't run node if
#if defined(ENABLE_EXTENSIONS)
    if (cmd_line->
        HasSwitch(::extensions::switches::kExtensionProcess))
      return false;
#endif
    return !(
      // node integration is disabled
      cmd_line->GetSwitchValueASCII(switches::kNodeIntegration) != "true" &&
      // and there is no preload script
      !cmd_line->HasSwitch(switches::kPreloadScript) &&
      !cmd_line->HasSwitch(switches::kPreloadURL));
  }

  static bool run_node() {
    return run_node(base::CommandLine::ForCurrentProcess());
  }

  // Modify the WebPreferences according to |web_contents|'s preferences.
  static void OverrideWebkitPrefs(
      content::WebContents* web_contents, content::WebPreferences* prefs);

  WebContentsPreferences(content::WebContents* web_contents,
                         const mate::Dictionary& web_preferences);
  ~WebContentsPreferences() override;

  // $.extend(|web_preferences_|, |new_web_preferences|).
  void Merge(const base::DictionaryValue& new_web_preferences);

  // Returns the web preferences.
  base::DictionaryValue* web_preferences() { return &web_preferences_; }

 private:
  friend class content::WebContentsUserData<WebContentsPreferences>;

  static std::vector<WebContentsPreferences*> instances_;

  content::WebContents* web_contents_;
  base::DictionaryValue web_preferences_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsPreferences);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
