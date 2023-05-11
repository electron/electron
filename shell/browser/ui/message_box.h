// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_MESSAGE_BOX_H_
#define ELECTRON_SHELL_BROWSER_UI_MESSAGE_BOX_H_

#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

class NativeWindow;

enum class MessageBoxType {
  kNone = 0,
  kInformation,
  kWarning,
  kError,
  kQuestion,
};

struct MessageBoxSettings {
  RAW_PTR_EXCLUSION electron::NativeWindow* parent_window = nullptr;
  MessageBoxType type = electron::MessageBoxType::kNone;
  std::vector<std::string> buttons;
  absl::optional<int> id;
  int default_id;
  int cancel_id;
  bool no_link = false;
  std::string title;
  std::string message;
  std::string detail;
  std::string checkbox_label;
  bool checkbox_checked = false;
  gfx::ImageSkia icon;
  int text_width = 0;

  MessageBoxSettings();
  MessageBoxSettings(const MessageBoxSettings&);
  ~MessageBoxSettings();
};

int ShowMessageBoxSync(const MessageBoxSettings& settings);

typedef base::OnceCallback<void(int code, bool checkbox_checked)>
    MessageBoxCallback;

void ShowMessageBox(const MessageBoxSettings& settings,
                    MessageBoxCallback callback);

void CloseMessageBox(int id);

// Like ShowMessageBox with simplest settings, but safe to call at very early
// stage of application.
void ShowErrorBox(const std::u16string& title, const std::u16string& content);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_MESSAGE_BOX_H_
