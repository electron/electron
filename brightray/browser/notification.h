// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BROWSER_NOTIFICATION_H_
#define BROWSER_NOTIFICATION_H_

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"

class SkBitmap;

namespace brightray {

class Notification {
 public:
  Notification() : weak_factory_(this) {}

  virtual void ShowNotification(const base::string16& title,
                                const base::string16& msg,
                                const SkBitmap& icon) = 0;
  virtual void DismissNotification() = 0;

  base::WeakPtr<Notification> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 protected:
  virtual ~Notification() {}

 private:
  base::WeakPtrFactory<Notification> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace brightray

#endif  // BROWSER_NOTIFICATION_H_
