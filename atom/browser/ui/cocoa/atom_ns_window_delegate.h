// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_ATOM_NS_WINDOW_DELEGATE_H_
#define ATOM_BROWSER_UI_COCOA_ATOM_NS_WINDOW_DELEGATE_H_

#include <Quartz/Quartz.h>

#include "ui/views/cocoa/views_nswindow_delegate.h"

namespace atom {
class NativeWindowMac;
}

@interface AtomNSWindowDelegate :
    ViewsNSWindowDelegate<NSTouchBarDelegate,
                          QLPreviewPanelDataSource> {
 @private
  atom::NativeWindowMac* shell_;
  bool is_zooming_;
  int level_;
  bool is_resizable_;
}
- (id)initWithShell:(atom::NativeWindowMac*)shell;
@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_NS_WINDOW_DELEGATE_H_
