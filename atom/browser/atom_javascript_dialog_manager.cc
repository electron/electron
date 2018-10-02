// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_javascript_dialog_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/message_box.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/options_switches.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/image/image_skia.h"

using content::JavaScriptDialogType;

namespace atom {

namespace {

constexpr int kUserWantsNoMoreDialogs = -1;

}  // namespace

AtomJavaScriptDialogManager::AtomJavaScriptDialogManager(
    api::WebContents* api_web_contents)
    : api_web_contents_(api_web_contents) {}
AtomJavaScriptDialogManager::~AtomJavaScriptDialogManager() = default;

void AtomJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* rfh,
    JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  auto origin_url = rfh->GetLastCommittedURL();
  const std::string& origin = origin_url.GetOrigin().spec();
  if (origin_counts_[origin] == kUserWantsNoMoreDialogs) {
    return std::move(callback).Run(false, base::string16());
  }

  if (dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_ALERT &&
      dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    std::move(callback).Run(false, base::string16());
    return;
  }

  std::vector<std::string> buttons = {"OK"};
  if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    buttons.push_back("Cancel");
  }

  origin_counts_[origin]++;

  auto* web_preferences = WebContentsPreferences::From(web_contents);
  std::string checkbox;
  if (origin_counts_[origin] > 1 && web_preferences &&
      web_preferences->IsEnabled("safeDialogs") &&
      !web_preferences->GetPreference("safeDialogsMessage", &checkbox)) {
    checkbox = "Prevent this app from creating additional dialogs";
  }

  // Don't set parent for offscreen window.
  NativeWindow* window = nullptr;
  if (web_preferences && !web_preferences->IsEnabled(options::kOffscreen)) {
    auto* relay = NativeWindowRelay::FromWebContents(web_contents);
    if (relay)
      window = relay->GetNativeWindow();
  }

  atom::ShowMessageBox(
      window, atom::MessageBoxType::MESSAGE_BOX_TYPE_NONE, buttons, -1, 0,
      atom::MessageBoxOptions::MESSAGE_BOX_NONE, "",
      base::UTF16ToUTF8(message_text), "", checkbox, false, gfx::ImageSkia(),
      base::Bind(&AtomJavaScriptDialogManager::OnMessageBoxCallback,
                 base::Unretained(this), base::Passed(std::move(callback)),
                 origin));
}

void AtomJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* rfh,
    bool is_reload,
    DialogClosedCallback callback) {
  bool default_prevented = api_web_contents_->Emit("will-prevent-unload");
  std::move(callback).Run(default_prevented, base::string16());
  return;
}

void AtomJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
    bool reset_state) {}

void AtomJavaScriptDialogManager::OnMessageBoxCallback(
    DialogClosedCallback callback,
    const std::string& origin,
    int code,
    bool checkbox_checked) {
  if (checkbox_checked)
    origin_counts_[origin] = kUserWantsNoMoreDialogs;
  std::move(callback).Run(code == 0, base::string16());
}

}  // namespace atom
