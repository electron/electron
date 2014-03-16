// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/atom_javascript_dialog_manager.h"

#include "base/strings/utf_string_conversions.h"

namespace atom {

void AtomJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    const GURL& origin_url,
    const std::string& accept_lang,
    content::JavaScriptMessageType javascript_message_type,
    const string16& message_text,
    const string16& default_prompt_text,
    const DialogClosedCallback& callback,
    bool* did_suppress_message) {
  callback.Run(false, string16());
}

void AtomJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    const string16& message_text,
    bool is_reload,
    const DialogClosedCallback& callback) {

  bool prevent_reload = message_text.empty() ||
                        message_text == ASCIIToUTF16("false");
  callback.Run(!prevent_reload, message_text);
}

}  // namespace atom
