// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_preferences.h"

namespace atom {

namespace {

const char* kWebPreferencesKey = "WebContentsPreferences";

}  // namespace

WebContentsPreferences::WebContentsPreferences(
    content::WebContents* web_contents,
    base::DictionaryValue&& web_preferences) {
  web_preferences_.Swap(&web_preferences);
  web_contents->SetUserData(kWebPreferencesKey, this);
}

WebContentsPreferences::~WebContentsPreferences() {
}

// static
WebContentsPreferences* WebContentsPreferences::From(
    content::WebContents* web_contents) {
  return static_cast<WebContentsPreferences*>(
      web_contents->GetUserData(kWebPreferencesKey));
}

}  // namespace atom
