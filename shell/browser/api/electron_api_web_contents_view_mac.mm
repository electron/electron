// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_contents_view.h"

#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>

namespace {

// Associated-object key tagging the specific NSView instance backing a
// WebContents' rendered content as non-interactive. Object identity is what
// matters, not the value stored under it.
const void* kInteractiveKey = &kInteractiveKey;

// Cached IMP for -[NSView hitTest:]'s original implementation, resolved once
// in EnsureHitTestSwizzled(). Do not look this up per-call — -hitTest: fires
// on essentially every mouse interaction across every NSView in the process.
NSView* (*g_original_hit_test)(id, SEL, NSPoint) = nullptr;

NSView* ElectronHitTest(id self_view, SEL _cmd, NSPoint point) {
  NSNumber* interactive = objc_getAssociatedObject(self_view, kInteractiveKey);
  if (interactive && !interactive.boolValue)
    return nil;  // Not part of this click — let AppKit try our siblings.

  return g_original_hit_test(self_view, _cmd, point);
}

void EnsureHitTestSwizzled() {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    Class cls = [NSView class];
    SEL selector = @selector(hitTest:);
    Method original_method = class_getInstanceMethod(cls, selector);

    g_original_hit_test = reinterpret_cast<NSView* (*)(id, SEL, NSPoint)>(
        method_getImplementation(original_method));

    class_replaceMethod(cls, selector, reinterpret_cast<IMP>(ElectronHitTest),
                        method_getTypeEncoding(original_method));
  });
}

}  // namespace

namespace electron::api {

void WebContentsView::ApplyInteractive() {
  if (!api_web_contents_ || !api_web_contents_->web_contents())
    return;

  NSView* content_view =
      api_web_contents_->web_contents()->GetNativeView().GetNativeNSView();
  if (!content_view)
    return;

  EnsureHitTestSwizzled();
  objc_setAssociatedObject(content_view, kInteractiveKey, @(GetInteractive()),
                           OBJC_ASSOCIATION_RETAIN);
}

}  // namespace electron::api