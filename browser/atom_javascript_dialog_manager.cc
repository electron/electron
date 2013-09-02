// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_javascript_dialog_manager.h"

#include "base/utf_string_conversions.h"

namespace atom {

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
