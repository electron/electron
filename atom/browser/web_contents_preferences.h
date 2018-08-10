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
  // Get self from WebContents.
  static WebContentsPreferences* From(content::WebContents* web_contents);

  WebContentsPreferences(content::WebContents* web_contents,
                         const mate::Dictionary& web_preferences);
  ~WebContentsPreferences() override;

  // A simple way to know whether a Boolean property is enabled.
  bool IsEnabled(const base::StringPiece& name, bool default_value = false);

  // $.extend(|web_preferences|, |new_web_preferences|).
  void Merge(const base::DictionaryValue& new_web_preferences);

  // Append command paramters according to preferences.
  void AppendCommandLineSwitches(base::CommandLine* command_line);

  // Modify the WebPreferences according to preferences.
  void OverrideWebkitPrefs(content::WebPreferences* prefs);

  // Returns the preload script path.
  bool GetPreloadPath(base::FilePath::StringType* path) const;

  // Returns the web preferences.
  base::DictionaryValue* dict() { return &dict_; }
  const base::DictionaryValue* dict() const { return &dict_; }
  base::DictionaryValue* last_dict() { return &last_dict_; }

 private:
  friend class content::WebContentsUserData<WebContentsPreferences>;
  friend class AtomBrowserClient;

  // Get WebContents according to process ID.
  static content::WebContents* GetWebContentsFromProcessID(int process_id);

  // Set preference value to given bool if user did not provide value
  bool SetDefaultBoolIfUndefined(const base::StringPiece& key, bool val);

  // Get preferences value as integer possibly coercing it from a string
  bool GetInteger(const base::StringPiece& attribute_name, int* val);

  static std::vector<WebContentsPreferences*> instances_;

  content::WebContents* web_contents_;

  base::DictionaryValue dict_;
  base::DictionaryValue last_dict_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsPreferences);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_PREFERENCES_H_
