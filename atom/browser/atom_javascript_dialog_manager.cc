// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_javascript_dialog_manager.h"

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/message_box.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/image/image_skia.h"

using content::JavaScriptDialogType;

namespace atom {

AtomJavaScriptDialogManager::AtomJavaScriptDialogManager(
    api::WebContents* api_web_contents)
    : api_web_contents_(api_web_contents) {}

void AtomJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    const GURL& origin_url,
    JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    const DialogClosedCallback& callback,
    bool* did_suppress_message) {
  if (dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_ALERT &&
      dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    callback.Run(false, base::string16());
    return;
  }

  std::vector<std::string> buttons = {"OK"};
  if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
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
  bool default_prevented = api_web_contents_->Emit("will-prevent-unload");
  callback.Run(default_prevented, base::string16());
  return;
}

void AtomJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
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
