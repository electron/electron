// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/inspectable_web_contents_view_mac.h"

#import <AppKit/AppKit.h>

#include "base/strings/sys_string_conversions.h"
#import "shell/browser/ui/cocoa/electron_inspectable_web_contents_view.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view_delegate.h"

namespace electron {

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContents* inspectable_web_contents) {
  return new InspectableWebContentsViewMac(inspectable_web_contents);
}

InspectableWebContentsViewMac::InspectableWebContentsViewMac(
    InspectableWebContents* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      view_([[ElectronInspectableWebContentsView alloc]
          initWithInspectableWebContentsViewMac:this]) {}

InspectableWebContentsViewMac::~InspectableWebContentsViewMac() {
  CloseDevTools();
}

gfx::NativeView InspectableWebContentsViewMac::GetNativeView() const {
  return view_.get();
}

void InspectableWebContentsViewMac::ShowDevTools(bool activate) {
  [view_ setDevToolsVisible:YES activate:activate];
}

void InspectableWebContentsViewMac::CloseDevTools() {
  [view_ setDevToolsVisible:NO activate:NO];
}

bool InspectableWebContentsViewMac::IsDevToolsViewShowing() {
  return [view_ isDevToolsVisible];
}

bool InspectableWebContentsViewMac::IsDevToolsViewFocused() {
  return [view_ isDevToolsFocused];
}

void InspectableWebContentsViewMac::SetIsDocked(bool docked, bool activate) {
  [view_ setIsDocked:docked activate:activate];
}

void InspectableWebContentsViewMac::SetContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  [view_ setContentsResizingStrategy:strategy];
}

void InspectableWebContentsViewMac::SetTitle(const std::u16string& title) {
  [view_ setTitle:base::SysUTF16ToNSString(title)];
}

}  // namespace electron
