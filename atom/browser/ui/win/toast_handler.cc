// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/toast_handler.h"

#include "atom/browser/ui/win/toast_lib.h"
#include "atom/browser/api/atom_api_notification.h"

namespace atom {

AtomToastHandler::AtomToastHandler(atom::api::Notification* target) {
  observer_ = target;
}

void AtomToastHandler::toastActivated() {
  observer_->OnClicked();
}

void AtomToastHandler::toastDismissed(
  WinToastLib::WinToastHandler::WinToastDismissalReason state) {
  // observer_->OnDismissed();
}

void AtomToastHandler::toastFailed() {
  // observer_->OnErrored();
}

}  // namespace atom
