// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Patrick Reynolds <piki@github.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/win/notification_presenter_win.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/files/file_enumerator.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "common/application_info.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "third_party/skia/include/core/SkBitmap.h"
#include "base/win/windows_version.h"

// Windows Header
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wrl/client.h>
#include <windows.ui.notifications.h>
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
  std::string appName = GetApplicationName();
  char* img = NULL;
  
  WinToasts::WindowsToastNotification* wtn = new WinToasts::WindowsToastNotification(appName.c_str());
  wtn->ShowNotification(title.c_str(), body.c_str(), img);
}

void NotificationPresenterWin::CancelNotification() {
}

void NotificationPresenterWin::DeleteNotification() {
}

}  // namespace brightray