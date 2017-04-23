// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/toast_lib.h"
#include "atom/browser/api/atom_api_notification.h"

#ifndef ATOM_BROWSER_UI_TOAST_HANDLER_H_
#define ATOM_BROWSER_UI_TOAST_HANDLER_H_

namespace atom {

class AtomToastHandler : public WinToastLib::WinToastHandler {
 public:
  atom::api::Notification* observer_;
  AtomToastHandler(atom::api::Notification* target);

  void toastActivated() override;
  void toastDismissed(WinToastLib::WinToastHandler::WinToastDismissalReason state);
  void toastFailed();
};

}

#endif  // ATOM_BROWSER_UI_TOAST_HANDLER_H_
