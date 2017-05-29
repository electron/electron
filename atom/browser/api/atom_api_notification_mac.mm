// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#import <Foundation/Foundation.h>

#include "atom/browser/browser.h"
#include "atom/browser/ui/notification_delegate_adapter.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "brightray/browser/notification.h"
#include "brightray/browser/notification_presenter.h"
#include "brightray/browser/mac/notification_presenter_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/gurl.h"

namespace atom {

namespace api {

brightray::NotificationPresenterMac* presenter;

void Notification::Show() {
  SkBitmap image = *(new SkBitmap);
  if (has_icon_) {
    image = *(icon_.ToSkBitmap());
  }

  std::unique_ptr<AtomNotificationDelegateAdapter> adapter(
      new AtomNotificationDelegateAdapter(this));
  auto notif = presenter->CreateNotification(adapter.get());
  if (notif) {
    ignore_result(adapter.release());  // it will release itself automatically.
    GURL nullUrl = *(new GURL);
    notif->Show(title_, body_, "", nullUrl, image, silent_, has_reply_, reply_placeholder_);
  }
}

bool initialized_ = false;
void Notification::OnInitialProps() {
  if (!initialized_) {
    presenter = new brightray::NotificationPresenterMac;
    initialized_ = true;
  }
}

void Notification::NotifyPropsUpdated() {
}

}  // namespace api

}  // namespace atom