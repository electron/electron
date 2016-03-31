// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_MESSAGE_HANDLER_DELEGATE_H_
#define ATOM_BROWSER_UI_WIN_MESSAGE_HANDLER_DELEGATE_H_

#include <windows.h>

namespace atom {

class MessageHandlerDelegate {
 public:
  // Catch-all message handling and filtering. Called before
  // HWNDMessageHandler's built-in handling, which may pre-empt some
  // expectations in Views/Aura if messages are consumed. Returns true if the
  // message was consumed by the delegate and should not be processed further
  // by the HWNDMessageHandler. In this case, |result| is returned. |result| is
  // not modified otherwise.
  virtual bool PreHandleMSG(
      UINT message, WPARAM w_param, LPARAM l_param, LRESULT* result);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_MESSAGE_HANDLER_DELEGATE_H_
