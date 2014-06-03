// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_TRAY_ICON_OBSERVER_H_
#define ATOM_BROWSER_UI_TRAY_ICON_OBSERVER_H_

namespace atom {

class TrayIconObserver {
 public:
  virtual void OnClicked() {}

 protected:
  virtual ~TrayIconObserver() {}
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_TRAY_ICON_OBSERVER_H_
