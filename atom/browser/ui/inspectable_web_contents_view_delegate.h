// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ATOM_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
#define ATOM_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_

#include <string>

#include "ui/gfx/image/image_skia.h"

namespace atom {

class InspectableWebContentsViewDelegate {
 public:
  virtual ~InspectableWebContentsViewDelegate() {}

  virtual void DevToolsFocused() {}
  virtual void DevToolsOpened() {}
  virtual void DevToolsClosed() {}

  // Returns the icon of devtools window.
  virtual gfx::ImageSkia GetDevToolsWindowIcon();

#if defined(USE_X11)
  // Called when creating devtools window.
  virtual void GetDevToolsWindowWMClass(std::string* name,
                                        std::string* class_name) {}
#endif
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
