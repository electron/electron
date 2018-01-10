// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_javascript_dialog_manager.h"

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/message_box.h"
#include "atom/browser/web_contents_preferences.h"
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
  const std::string origin = origin_url.GetOrigin().spec();
  if (origin_counts_.find(origin) == origin_counts_.end()) {
    origin_counts_[origin] = 0;
  }

  if (origin_counts_[origin] == -1) return callback.Run(false, base::string16());;

  if (dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_ALERT &&
      dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    callback.Run(false, base::string16());
    return;
  }

  std::vector<std::string> buttons = {"OK"};
  if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    buttons.push_back("Cancel");
  }

  origin_counts_[origin]++;

  std::string checkbox_string;
  if (origin_counts_[origin] > 1 &&
      WebContentsPreferences::IsPreferenceEnabled("safeDialogs", web_contents)) {
    if (!WebContentsPreferences::GetString("safeDialogsMessage",
                                           &checkbox_string, web_contents)) {
      checkbox_string = "Prevent this app from creating additional dialogs";
    }
  }
  atom::ShowMessageBox(NativeWindow::FromWebContents(web_contents),
                       atom::MessageBoxType::MESSAGE_BOX_TYPE_NONE, buttons, -1,
                       0, atom::MessageBoxOptions::MESSAGE_BOX_NONE, "",
                       base::UTF16ToUTF8(message_text), "", checkbox_string, false,
                       gfx::ImageSkia(),
                       base::Bind(&OnMessageBoxCallback, callback, origin,
                                  &origin_counts_));
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
    const std::string& origin,
    std::map<std::string, int>* origin_counts_,
    int code,
    bool checkbox_checked) {
  if (checkbox_checked) {
    (*origin_counts_)[origin] = -1;
  }
  callback.Run(code == 0, base::string16());
}

}  // namespace atom
