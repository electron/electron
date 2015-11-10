// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/win/notification_presenter_win.h"
#include "base/win/windows_version.h"
#include "common/application_info.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

#pragma comment(lib, "runtimeobject.lib")

using namespace WinToasts;
using namespace Microsoft::WRL;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::Foundation;

namespace brightray {

// static
NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterWin;
}

NotificationPresenterWin::NotificationPresenterWin() {
  m_lastNotification = nullptr;
}

NotificationPresenterWin::~NotificationPresenterWin() {
}

void NotificationPresenterWin::ShowNotification(
    const content::PlatformNotificationData& data,
    const SkBitmap& icon,
    scoped_ptr<content::DesktopNotificationDelegate> delegate_ptr,
    base::Closure* cancel_callback) {
  std::wstring title = data.title;
  std::wstring body = data.body;
  std::string iconPath = data.icon.spec();
  std::string appName = GetApplicationName();

  // toast notification supported in version >= Windows 8
  // for prior versions, use Tray.displayBalloon
  if (base::win::GetVersion() >= base::win::VERSION_WIN8) {
    wtn = new WindowsToastNotification(appName.c_str(), delegate_ptr.release());
    wtn->ShowNotification(title.c_str(), body.c_str(), iconPath, m_lastNotification);
  }

  if (cancel_callback) {
    *cancel_callback = base::Bind(
      &NotificationPresenterWin::RemoveNotification,
      base::Unretained(this));
  }
}

void NotificationPresenterWin::RemoveNotification() {
  if (m_lastNotification != nullptr && wtn != NULL) {
    wtn->DismissNotification(m_lastNotification);
  }
}

}  // namespace brightray
