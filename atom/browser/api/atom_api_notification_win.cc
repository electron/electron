// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#include "atom/browser/browser.h"
#include "atom/browser/ui/notification_delegate_adapter.h"
#include "atom/browser/ui/win/toast_handler.h"
#include "atom/browser/ui/win/toast_lib.h"
#include "base/files/file_util.h"
#include "base/md5.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/notification.h"
#include "browser/notification_presenter.h"
#include "browser/win/notification_presenter_win.h"
#include "browser/win/notification_presenter_win7.h"
#include "common/string_conversion.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/gurl.h"

namespace atom {

namespace api {

bool can_toast_ = true;
bool initialized_ = false;
brightray::NotificationPresenterWin7* presenter;

base::ScopedTempDir temp_dir_;

bool SaveIconToPath(const SkBitmap& bitmap, const base::FilePath& path) {
  std::vector<unsigned char> png_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &png_data))
    return false;

  char* data = reinterpret_cast<char*>(&png_data[0]);
  int size = static_cast<int>(png_data.size());
  return base::WriteFile(path, data, size) == size;
}

void Notification::Show() {
  SkBitmap image = *(new SkBitmap);
  if (has_icon_) {
    image = *(icon_.ToSkBitmap());
  }

  if (can_toast_) {
    atom::AtomToastHandler* handler = new atom::AtomToastHandler(this);
    WinToastLib::WinToastTemplate::WinToastTemplateType toastType =
        WinToastLib::WinToastTemplate::TextOneLine;
    if (!has_icon_) {
      if (body_ != L"") {
        toastType = WinToastLib::WinToastTemplate::TextTwoLines;
      } else {
        toastType = WinToastLib::WinToastTemplate::TextOneLine;
      }
    } else {
      if (body_ != L"") {
        toastType = WinToastLib::WinToastTemplate::ImageWithTwoLines;
      } else {
        toastType = WinToastLib::WinToastTemplate::ImageWithOneLine;
      }
    }
    WinToastLib::WinToastTemplate toast =
        WinToastLib::WinToastTemplate(toastType);

    std::string filename =
        base::MD5String(base::UTF16ToUTF8(icon_path_)) + ".png";
    base::FilePath savePath =
        temp_dir_.GetPath().Append(base::UTF8ToUTF16(filename));
    if (has_icon_ && SaveIconToPath(image, savePath)) {
      toast.setImagePath(savePath.value());
    }
    toast.setTextField(title_,
                       WinToastLib::WinToastTemplate::TextField::FirstLine);
    toast.setTextField(body_,
                       WinToastLib::WinToastTemplate::TextField::SecondLine);
    toast.setSilent(silent_);

    WinToastLib::WinToast::instance()->showToast(toast, handler);

    OnShown();
  } else {
    AtomNotificationDelegateAdapter* adapter =
        new AtomNotificationDelegateAdapter(this);
    auto notif = presenter->CreateNotification(adapter);
    GURL nullUrl = *(new GURL);
    notif->Show(title_, body_, "", nullUrl, image, silent_);
  }
}

void Notification::OnInitialProps() {
  if (!initialized_) {
    Browser* browser = Browser::Get();
    WinToastLib::WinToast::instance()->setAppName(
        base::UTF8ToUTF16(browser->GetName()));
    WinToastLib::WinToast::instance()->setAppUserModelId(
        browser->GetAppUserModelID());
    can_toast_ = WinToastLib::WinToast::instance()->initialize();
    temp_dir_.CreateUniqueTempDir();
  }
  if (!can_toast_) {
    presenter = new brightray::NotificationPresenterWin7;
  }
}

void Notification::NotifyPropsUpdated() {}
}  // namespace api
}  // namespace atom
