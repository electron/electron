// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ELECTRON_SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_

#include <string>

#include "ui/base/models/image_model.h"

namespace electron {

class InspectableWebContentsViewDelegate {
 public:
  virtual ~InspectableWebContentsViewDelegate() {}

  virtual void DevToolsFocused() {}
  virtual void DevToolsOpened() {}
  virtual void DevToolsClosed() {}
  virtual void DevToolsResized() {}

  // Returns the icon of devtools window.
  virtual ui::ImageModel GetDevToolsWindowIcon();

#if defined(OS_LINUX)
  // Called when creating devtools window.
  virtual void GetDevToolsWindowWMClass(std::string* name,
                                        std::string* class_name) {}
#endif
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
