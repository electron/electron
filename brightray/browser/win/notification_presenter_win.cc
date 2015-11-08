// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jpoon@microsoft.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/win/notification_presenter_win.h"
#include "base/strings/string_util.h"
#include "base/win/windows_version.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "common/application_info.h"

#include <stdlib.h>
#include <stdio.h>
#include <vector>

// Windows Header
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <winstring.h>
#include "windows_toast_notification.h"

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "Crypt32.lib")

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
  
  WinToasts::WindowsToastNotification* wtn = new WinToasts::WindowsToastNotification(appName.c_str(), delegate_ptr.release());
  wtn->ShowNotification(title.c_str(), body.c_str(), iconPath);
}

void NotificationPresenterWin::CancelNotification() {
  // No can do.
}

void NotificationPresenterWin::DeleteNotification() {
  // No can do.
}

}  // namespace brightray