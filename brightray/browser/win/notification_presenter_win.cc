// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/win/notification_presenter_win.h"

#include "base/files/file_util.h"
#include "base/md5.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "browser/win/windows_toast_notification.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

#pragma comment(lib, "runtimeobject.lib")

namespace brightray {

namespace {

bool SaveIconToPath(const SkBitmap& bitmap, const base::FilePath& path) {
  std::vector<unsigned char> png_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &png_data))
    return false;

  char* data = reinterpret_cast<char*>(&png_data[0]);
  int size = static_cast<int>(png_data.size());
  return base::WriteFile(path, data, size) == size;
}

void RemoveNotification(base::WeakPtr<WindowsToastNotification> notification) {
  if (notification)
    notification->DismissNotification();
}

}  // namespace

// static
NotificationPresenter* NotificationPresenter::Create() {
  if (!WindowsToastNotification::Initialize())
    return nullptr;
  scoped_ptr<NotificationPresenterWin> presenter(new NotificationPresenterWin);
  if (!presenter->Init())
    return nullptr;
  return presenter.release();
}

NotificationPresenterWin::NotificationPresenterWin() {
}

NotificationPresenterWin::~NotificationPresenterWin() {
}

bool NotificationPresenterWin::Init() {
  return temp_dir_.CreateUniqueTempDir();
}

void NotificationPresenterWin::ShowNotification(
    const content::PlatformNotificationData& data,
    const SkBitmap& icon,
    scoped_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  // This class manages itself.
  auto notification = new WindowsToastNotification(delegate.Pass());
  notification->ShowNotification(
      data.title, data.body, SaveIconToFilesystem(icon, data.icon));

  if (cancel_callback) {
    *cancel_callback = base::Bind(
        &RemoveNotification, notification->GetWeakPtr());
  }
}

std::wstring NotificationPresenterWin::SaveIconToFilesystem(
    const SkBitmap& icon, const GURL& origin) {
  std::string filename = base::MD5String(origin.spec()) + ".png";
  base::FilePath path = temp_dir_.path().Append(base::UTF8ToUTF16(filename));
  if (base::PathExists(path))
    return path.value();
  if (SaveIconToPath(icon, path))
    return path.value();
  return base::UTF8ToUTF16(origin.spec());
}

}  // namespace brightray
