// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "browser/api/atom_api_menu_mac.h"

#include "base/message_loop.h"
#include "base/mac/scoped_sending_event.h"
#include "browser/native_window.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"

namespace atom {

namespace api {

MenuMac::MenuMac(v8::Handle<v8::Object> wrapper)
    : Menu(wrapper),
      controller_([[MenuController alloc] initWithModel:model_.get()
                                 useWithPopUpButtonCell:NO]) {
}

MenuMac::~MenuMac() {
}

void MenuMac::Popup(NativeWindow* native_window, int ox, int oy) {
  NSWindow* window = native_window->GetNativeWindow();
  content::WebContents* web_contents = native_window->GetWebContents();

  CGFloat x = ox;
  CGFloat y = oy;

  // Fake out a context menu event.
  NSEvent* currentEvent = [NSApp currentEvent];
  NSView* web_view = web_contents->GetView()->GetNativeView();
  NSPoint position = { x, web_view.bounds.size.height - y };
  NSTimeInterval eventTime = [currentEvent timestamp];
  NSEvent* clickEvent = [NSEvent mouseEventWithType:NSRightMouseDown
                                           location:position
                                      modifierFlags:NSRightMouseDownMask
                                          timestamp:eventTime
                                       windowNumber:[window windowNumber]
                                            context:nil
                                        eventNumber:0
                                         clickCount:1
                                           pressure:1.0];

  {
    // Make sure events can be pumped while the menu is up.
    MessageLoop::ScopedNestableTaskAllower allow(MessageLoop::current());

    // One of the events that could be pumped is |window.close()|.
    // User-initiated event-tracking loops protect against this by
    // setting flags in -[CrApplication sendEvent:], but since
    // web-content menus are initiated by IPC message the setup has to
    // be done manually.
    base::mac::ScopedSendingEvent sendingEventScoper;

    // Show the menu.
    [NSMenu popUpContextMenu:[controller_ menu]
                   withEvent:clickEvent
                     forView:web_view];
  }
}

// static
Menu* Menu::Create(v8::Handle<v8::Object> wrapper) {
  return new MenuMac(wrapper);
}

}  // namespace api

}  // namespace atom
