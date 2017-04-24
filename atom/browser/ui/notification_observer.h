// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_NOTIFICATION_OBSERVER_H_
#define ATOM_BROWSER_UI_NOTIFICATION_OBSERVER_H_

#include <string>

namespace atom {

class NotifictionObserver {
 public:
  virtual void OnClicked() {}
  virtual void OnReplied(std::string reply) {}
  virtual void OnShown() {}

 protected:
  virtual ~NotifictionObserver() {}
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_NOTIFICATION_OBSERVER_H_
