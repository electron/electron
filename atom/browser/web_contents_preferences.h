// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
#define ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"

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

  static bool IsPreferenceEnabled(const std::string& attribute_name,
                                  content::WebContents* web_contents);
  static bool IsSandboxed(content::WebContents* web_contents);
  static bool UsesNativeWindowOpen(content::WebContents* web_contents);
  static bool DisablePopups(content::WebContents* web_contents);
  static bool IsPluginsEnabled(content::WebContents* web_contents);

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

  // Get preferences value as integer possibly coercing it from a string
  bool GetInteger(const std::string& attributeName, int* intValue);

  DISALLOW_COPY_AND_ASSIGN(WebContentsPreferences);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
