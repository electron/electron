// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_javascript_dialog_manager.h"

#include <string>
#include <vector>

#include "atom/browser/native_window.h"
#include "atom/browser/ui/message_box.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/image/image_skia.h"

using content::JavaScriptMessageType;

namespace atom {

void AtomJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    const GURL& origin_url,
    JavaScriptMessageType message_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    const DialogClosedCallback& callback,
    bool* did_suppress_message) {

  if (message_type != JavaScriptMessageType::JAVASCRIPT_MESSAGE_TYPE_ALERT &&
      message_type != JavaScriptMessageType::JAVASCRIPT_MESSAGE_TYPE_CONFIRM) {
    callback.Run(false, base::string16());
    return;
  }

  std::vector<std::string> buttons = {"OK"};
  if (message_type == JavaScriptMessageType::JAVASCRIPT_MESSAGE_TYPE_CONFIRM) {
    buttons.push_back("Cancel");
  }

  atom::ShowMessageBox(NativeWindow::FromWebContents(web_contents),
                       atom::MessageBoxType::MESSAGE_BOX_TYPE_NONE, buttons, -1,
                       0, atom::MessageBoxOptions::MESSAGE_BOX_NONE, "",
                       base::UTF16ToUTF8(message_text), "", "", false,
                       gfx::ImageSkia(),
                       base::Bind(&OnMessageBoxCallback, callback));
}

void AtomJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    bool is_reload,
    const DialogClosedCallback& callback) {
  // FIXME(zcbenz): the |message_text| is removed, figure out what should we do.
  callback.Run(false, base::ASCIIToUTF16("This should not be displayed"));
}

void AtomJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
    bool suppress_callbacks,
    bool reset_state) {
}

// static
void AtomJavaScriptDialogManager::OnMessageBoxCallback(
    const DialogClosedCallback& callback,
    int code,
    bool checkbox_checked) {
  callback.Run(code == 0, base::string16());
}

}  // namespace atom
