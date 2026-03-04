// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Felix Rieseberg <feriese@microsoft.com> and
// Jason Poon <jason.poon@microsoft.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/notifications/win/notification_presenter_win.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/hash/sha1.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "shell/browser/notifications/win/windows_toast_activator.h"
#include "shell/browser/notifications/win/windows_toast_notification.h"
#include "shell/common/thread_restrictions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

#pragma comment(lib, "runtimeobject.lib")

namespace electron {

namespace {

bool SaveIconToPath(const SkBitmap& bitmap, const base::FilePath& path) {
  std::optional<std::vector<uint8_t>> png_data =
      gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false);
  if (!png_data.has_value() || !png_data.value().size())
    return false;
  return base::WriteFile(path, png_data.value());
}

}  // namespace

// static
std::unique_ptr<NotificationPresenter> NotificationPresenter::Create() {
  if (!WindowsToastNotification::Initialize())
    return {};
  auto presenter = std::make_unique<NotificationPresenterWin>();
  if (!presenter->Init())
    return {};

  // Ensure COM toast activator is registered once the presenter is ready.
  NotificationActivator::RegisterActivator();

  if (electron::debug_notifications)
    LOG(INFO) << "Successfully created Windows notifications presenter";

  return presenter;
}

NotificationPresenterWin::NotificationPresenterWin() = default;

NotificationPresenterWin::~NotificationPresenterWin() = default;

bool NotificationPresenterWin::Init() {
  ScopedAllowBlockingForElectron allow_blocking;
  return temp_dir_.CreateUniqueTempDir();
}

std::wstring NotificationPresenterWin::SaveIconToFilesystem(
    const SkBitmap& icon,
    const GURL& origin) {
  if (icon.drawsNothing())
    return L"";

  std::string filename;
  if (origin.is_valid()) {
    filename = base::SHA1HashString(origin.spec()) + ".png";
  } else {
    const int64_t now_usec = base::Time::Now().since_origin().InMicroseconds();
    filename = base::NumberToString(now_usec) + ".png";
  }

  ScopedAllowBlockingForElectron allow_blocking;
  base::FilePath path = temp_dir_.GetPath().Append(base::UTF8ToWide(filename));

  if (!SaveIconToPath(icon, path))
    return L"";

  return path.value();
}

Notification* NotificationPresenterWin::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new WindowsToastNotification(delegate, this);
}

}  // namespace electron
