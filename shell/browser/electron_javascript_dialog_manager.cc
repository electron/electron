// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_javascript_dialog_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_process_host.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/message_box.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/options_switches.h"
#include "ui/gfx/image/image_skia.h"

using content::JavaScriptDialogType;

namespace electron {

namespace {

constexpr int kUserWantsNoMoreDialogs = -1;

}  // namespace

ElectronJavaScriptDialogManager::ElectronJavaScriptDialogManager() = default;
ElectronJavaScriptDialogManager::~ElectronJavaScriptDialogManager() = default;

void ElectronJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* rfh,
    JavaScriptDialogType dialog_type,
    const std::u16string& message_text,
    const std::u16string& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  auto origin_url = rfh->GetLastCommittedURL();

  std::string origin;
  // For file:// URLs we do the alert filtering by the
  // file path currently loaded
  if (origin_url.SchemeIsFile()) {
    origin = origin_url.path();
  } else {
    origin = origin_url.DeprecatedGetOriginAsURL().spec();
  }

  if (origin_counts_[origin] == kUserWantsNoMoreDialogs) {
    return std::move(callback).Run(false, std::u16string());
  }

  auto* web_preferences = WebContentsPreferences::From(web_contents);

  if (web_preferences && web_preferences->ShouldDisableDialogs()) {
    return std::move(callback).Run(false, std::u16string());
  }

  // We want to allow a second call (which is no-op) if the user calls the
  // event callback AND doesn't use preventDefault()
  using Helper =
      base::internal::OnceCallbackHolder<bool, const std::u16string&>;
  auto repeating_callback = base::BindRepeating(
      &Helper::Run, std::make_unique<Helper>(std::move(callback),
                                             /*ignore_extra_runs=*/true));

  bool default_prevented =
      EmitEvent(web_contents, rfh, dialog_type, message_text,
                default_prompt_text, repeating_callback);
  if (default_prevented)
    return;

  if (dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_ALERT &&
      dialog_type != JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM) {
    repeating_callback.Run(false, std::u16string());
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

  std::string checkbox;
  if (origin_counts_[origin] > 1 && web_preferences &&
      web_preferences->ShouldUseSafeDialogs() &&
      !web_preferences->GetSafeDialogsMessage(&checkbox)) {
    checkbox = "Prevent this app from creating additional dialogs";
  }

  // Don't set parent for offscreen window.
  NativeWindow* window = nullptr;
  if (web_preferences && !web_preferences->IsOffscreen()) {
    auto* relay = NativeWindowRelay::FromWebContents(web_contents);
    if (relay)
      window = relay->GetNativeWindow();
  }

  electron::MessageBoxSettings settings;
  settings.parent_window = window;
  settings.checkbox_label = checkbox;
  settings.buttons = buttons;
  settings.default_id = default_id;
  settings.cancel_id = cancel_id;
  settings.message = base::UTF16ToUTF8(message_text);

  electron::ShowMessageBox(
      settings,
      base::BindOnce(&ElectronJavaScriptDialogManager::OnMessageBoxCallback,
                     base::Unretained(this), repeating_callback, origin));
}

void ElectronJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    content::RenderFrameHost* rfh,
    bool is_reload,
    DialogClosedCallback callback) {
  auto* api_web_contents = api::WebContents::From(web_contents);
  if (api_web_contents) {
    bool default_prevented = api_web_contents->Emit("will-prevent-unload");
    std::move(callback).Run(default_prevented, std::u16string());
  }
}

void ElectronJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
    bool reset_state) {}

void ElectronJavaScriptDialogManager::OnMessageBoxCallback(
    DialogClosedCallback callback,
    const std::string& origin,
    int code,
    bool checkbox_checked) {
  if (checkbox_checked)
    origin_counts_[origin] = kUserWantsNoMoreDialogs;
  std::move(callback).Run(code == 0, std::u16string());
}

bool ElectronJavaScriptDialogManager::EmitEvent(
    content::WebContents* web_contents,
    content::RenderFrameHost* rfh,
    content::JavaScriptDialogType dialog_type,
    const std::u16string& message_text,
    const std::u16string& default_prompt_text,
    DialogClosedCallback callback) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  auto details = gin::Dictionary::CreateEmpty(isolate);
  details.Set("message", message_text);
  details.Set("frame", rfh);

  if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_ALERT)
    details.Set("type", "alert");
  else if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_CONFIRM)
    details.Set("type", "confirm");
  else if (dialog_type == JavaScriptDialogType::JAVASCRIPT_DIALOG_TYPE_PROMPT) {
    details.Set("type", "prompt");
    details.Set("defaultValue", default_prompt_text);
  } else
    details.Set("type", "");

  auto* api_web_contents = api::WebContents::From(web_contents);
  if (!api_web_contents)
    return false;

  return api_web_contents->Emit("dialog", details, std::move(callback));
}

}  // namespace electron
