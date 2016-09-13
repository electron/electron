// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_MESSAGE_BOX_H_
#define ATOM_BROWSER_UI_MESSAGE_BOX_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/strings/string16.h"

namespace gfx {
class ImageSkia;
}

namespace atom {

class NativeWindow;

enum MessageBoxType {
  MESSAGE_BOX_TYPE_NONE = 0,
  MESSAGE_BOX_TYPE_INFORMATION,
  MESSAGE_BOX_TYPE_WARNING,
  MESSAGE_BOX_TYPE_ERROR,
  MESSAGE_BOX_TYPE_QUESTION,
};

enum MessageBoxOptions {
  MESSAGE_BOX_NONE    = 0,
  MESSAGE_BOX_NO_LINK = 1 << 0,
};

typedef base::Callback<void(int code)> MessageBoxCallback;

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   int cancel_id,
                   int default_id,
                   int options,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail,
                   const gfx::ImageSkia& icon);

void ShowMessageBox(NativeWindow* parent_window,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    int default_id,
                    int cancel_id,
                    int options,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback);

// Like ShowMessageBox with simplest settings, but safe to call at very early
// stage of application.
void ShowErrorBox(const base::string16& title, const base::string16& content);

}  // namespace atom

#endif  // ATOM_BROWSER_UI_MESSAGE_BOX_H_
