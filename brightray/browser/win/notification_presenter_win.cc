// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/win/notification_presenter_win.h"

#include "base/win/windows_version.h"
#include "browser/win/windows_toast_notification.h"
#include "common/application_info.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

#pragma comment(lib, "runtimeobject.lib")

namespace brightray {

namespace {

void RemoveNotification(base::WeakPtr<WindowsToastNotification> notification) {
  if (notification)
    notification->DismissNotification();
}

}  // namespace

// static
NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterWin;
}

NotificationPresenterWin::NotificationPresenterWin() {
}

NotificationPresenterWin::~NotificationPresenterWin() {
}

void NotificationPresenterWin::ShowNotification(
    const content::PlatformNotificationData& data,
    const SkBitmap& icon,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  // This class manages itself.
  auto notification = new WindowsToastNotification(
      GetApplicationName(), delegate.Pass());
  notification->ShowNotification(data.title, data.body, data.icon.spec());

  if (cancel_callback) {
    *cancel_callback = base::Bind(
        &RemoveNotification, notification->GetWeakPtr());
  }
}

}  // namespace brightray
