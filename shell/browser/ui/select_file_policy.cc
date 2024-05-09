// Copyright (c) 2024 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/select_file_policy.h"

#include "base/logging.h"
#include "shell/browser/web_contents_preferences.h"

ElectronSelectFilePolicy::ElectronSelectFilePolicy(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

ElectronSelectFilePolicy::~ElectronSelectFilePolicy() = default;

bool ElectronSelectFilePolicy::CanOpenSelectFileDialog() {
  auto* web_preferences = electron::WebContentsPreferences::From(web_contents_);
  if (!web_preferences)
    return true;

  return !web_preferences->ShouldDisableDialogs();
}

// TODO(codebytere): Figure out how to handle this in Electron.
void ElectronSelectFilePolicy::SelectFileDenied() {
  LOG(INFO) << "Select file dialog denied";
}
