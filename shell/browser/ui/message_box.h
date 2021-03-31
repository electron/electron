// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_MESSAGE_BOX_H_
#define SHELL_BROWSER_UI_MESSAGE_BOX_H_

#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/strings/string16.h"
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

using DialogResult = std::pair<int, bool>;

struct MessageBoxSettings {
  electron::NativeWindow* parent_window = nullptr;
  MessageBoxType type = electron::MessageBoxType::kNone;
  std::vector<std::string> buttons;
  int default_id;
  int cancel_id;
  bool no_link = false;
  std::string title;
  std::string message;
  std::string detail;
  std::string checkbox_label;
  bool checkbox_checked = false;
  gfx::ImageSkia icon;

  MessageBoxSettings();
  MessageBoxSettings(const MessageBoxSettings&);
  ~MessageBoxSettings();
};

int ShowMessageBoxSync(const MessageBoxSettings& settings);

typedef base::OnceCallback<void(int code, bool checkbox_checked)>
    MessageBoxCallback;

void ShowMessageBox(const MessageBoxSettings& settings,
                    MessageBoxCallback callback);

// Like ShowMessageBox with simplest settings, but safe to call at very early
// stage of application.
void ShowErrorBox(const base::string16& title, const base::string16& content);

}  // namespace electron

#endif  // SHELL_BROWSER_UI_MESSAGE_BOX_H_
