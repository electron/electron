// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#include "atom/browser/ui/win/toast_handler.h"
#include "atom/browser/ui/win/toast_lib.h"
#include "base/strings/utf_string_conversions.h"
#include "common/string_conversion.h"

namespace atom {

namespace api {

bool can_toast_ = true;
bool initialized_ = false;

void Notification::Show() {
  atom::AtomToastHandler* handler = new atom::AtomToastHandler(this);
  WinToastLib::WinToastTemplate toast = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::TextTwoLines);
  // toast.setImagePath(L"C:\example.png");
  toast.setTextField(title_, WinToastLib::WinToastTemplate::TextField::FirstLine);
  toast.setTextField(body_, WinToastLib::WinToastTemplate::TextField::SecondLine);
  toast.setSilent(silent_);

  WinToastLib::WinToast::instance()->showToast(toast, handler);

  OnShown();
}

void Notification::OnInitialProps() {
  if (!initialized_) {
    WinToastLib::WinToast::instance()->setAppName(L"WinToastExample");
    WinToastLib::WinToast::instance()->setAppUserModelId(
                WinToastLib::WinToast::configureAUMI(L"mohabouje", L"wintoast", L"wintoastexample", L"20161006")
    );
    can_toast_ = WinToastLib::WinToast::instance()->initialize();
  }
}

void Notification::NotifyPropsUpdated() {
  
}

}

}