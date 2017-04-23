// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#include "atom/browser/ui/notification_delegate_adapter.h"
#include "atom/browser/ui/win/toast_handler.h"
#include "atom/browser/ui/win/toast_lib.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/notification.h"
#include "browser/notification_presenter.h"
#include "browser/win/notification_presenter_win7.h"
#include "common/string_conversion.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

namespace atom {

namespace api {

bool can_toast_ = true;
bool initialized_ = false;
brightray::NotificationPresenter* presenter;

void Notification::Show() {
  if (can_toast_) {
    atom::AtomToastHandler* handler = new atom::AtomToastHandler(this);
    WinToastLib::WinToastTemplate toast = WinToastLib::WinToastTemplate(WinToastLib::WinToastTemplate::TextTwoLines);
    // toast.setImagePath(L"C:\example.png");
    toast.setTextField(title_, WinToastLib::WinToastTemplate::TextField::FirstLine);
    toast.setTextField(body_, WinToastLib::WinToastTemplate::TextField::SecondLine);
    toast.setSilent(silent_);

    WinToastLib::WinToast::instance()->showToast(toast, handler);

    OnShown();
  } else {
    AtomNotificationDelegateAdapter* adapter = new AtomNotificationDelegateAdapter(this);
    auto notif = presenter->CreateNotification(adapter);
    GURL* u = new GURL;
    notif->Show(
      title_,
      body_,
      "",
      u->Resolve(""),
      *(new SkBitmap),
      true
    );
  }
}

void Notification::OnInitialProps() {
  if (!initialized_) {
    WinToastLib::WinToast::instance()->setAppName(L"WinToastExample");
    WinToastLib::WinToast::instance()->setAppUserModelId(
                WinToastLib::WinToast::configureAUMI(L"mohabouje", L"wintoast", L"wintoastexample", L"20161006")
    );
    can_toast_ = WinToastLib::WinToast::instance()->initialize();
  }
  can_toast_ = false;
  if (!can_toast_) {
    presenter = new brightray::NotificationPresenterWin7;
  }
}

void Notification::NotifyPropsUpdated() {
  
}

}

}