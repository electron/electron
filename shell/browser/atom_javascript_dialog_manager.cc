// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_javascript_dialog_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/api/atom_api_web_contents.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/message_box.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/options_switches.h"
#include "ui/gfx/image/image_skia.h"

using content::JavaScriptDialogType;

namespace electron {

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

  std::string origin;
  // For file:// URLs we do the alert filtering by the
  // file path currently loaded
  if (origin_url.SchemeIsFile()) {
    origin = origin_url.path();
  } else {
    origin = origin_url.GetOrigin().spec();
  }

  if (origin_counts_[origin] == kUserWantsNoMoreDialogs) {
    return std::move(callback).Run(false, base::string16());
  }

  if (dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_ALERT &&
      dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    std::move(callback).Run(false, base::string16());
    return;
  }

  // No default button
  int default_id = -1;
  int cancel_id = 0;

  std::vector<std::string> buttons = {"OK"};
  if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    buttons.emplace_back("Cancel");
    // First button is default, second button is cancel
    default_id = 0;
    cancel_id = 1;
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

  electron::MessageBoxSettings settings;
  settings.parent_window = window;
  settings.buttons = buttons;
  settings.default_id = default_id;
  settings.cancel_id = cancel_id;
  settings.message = base::UTF16ToUTF8(message_text);

  electron::ShowMessageBox(
      settings,
      base::BindOnce(&AtomJavaScriptDialogManager::OnMessageBoxCallback,
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

}  // namespace electron
