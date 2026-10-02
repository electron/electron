// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents_view.h"

#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>

#include <string>

#include "shell/browser/api/electron_api_web_contents.h"

namespace {

// The native WebContents view can receive mouse input without passing through
// the Views targeter. Returning nil lets AppKit continue hit testing the
// sibling views beneath it in the same window.
NSView* IgnoreMouseHitTest(id, SEL, NSPoint) {
  return nil;
}

Class GetOrCreateIgnoringMouseClass(Class original_class) {
  const std::string name = std::string(class_getName(original_class)) +
                           "_ElectronIgnoringMouseEvents";
  if (Class existing = objc_lookUpClass(name.c_str()))
    return existing;

  Class subclass = objc_allocateClassPair(original_class, name.c_str(), 0);
  if (!subclass)
    return Nil;

  Method hit_test =
      class_getInstanceMethod(original_class, @selector(hitTest:));
  if (!hit_test || !class_addMethod(subclass, @selector(hitTest:),
                                    reinterpret_cast<IMP>(IgnoreMouseHitTest),
                                    method_getTypeEncoding(hit_test))) {
    objc_disposeClassPair(subclass);
    return Nil;
  }

  // Preserve the class reported to callers, as other AppKit dynamic subclasses
  // do when substituting an object's actual class.
  class_addMethod(subclass, @selector(class),
                  imp_implementationWithBlock(^Class(id) {
                    return original_class;
                  }),
                  "#@:");
  objc_registerClassPair(subclass);
  return subclass;
}

char original_class_key;

void SetNativeViewIgnoresMouseEvents(NSView* view, bool ignore) {
  if (!view)
    return;

  Class saved_class = objc_getAssociatedObject(view, &original_class_key);
  if (!ignore) {
    if (saved_class) {
      object_setClass(view, saved_class);
      objc_setAssociatedObject(view, &original_class_key, nil,
                               OBJC_ASSOCIATION_ASSIGN);
    }
    return;
  }

  if (saved_class)
    return;

  Class original_class = object_getClass(view);
  if (Class subclass = GetOrCreateIgnoringMouseClass(original_class)) {
    // An association is removed with the view if WebContents is destroyed
    // externally, unlike a process-wide map keyed by a possibly stale NSView*.
    // Class objects live for the process lifetime, so an assign association is
    // sufficient and disappears automatically when the NSView is deallocated.
    objc_setAssociatedObject(view, &original_class_key, original_class,
                             OBJC_ASSOCIATION_ASSIGN);
    object_setClass(view, subclass);
  }
}

}  // namespace

namespace electron::api {

void WebContentsView::ApplyIgnoreMouseEvents() {
  auto* web_contents = GetLiveWebContents();
  if (!web_contents)
    return;

  NSView* native_view =
      web_contents->web_contents()->GetNativeView().GetNativeNSView();
  SetNativeViewIgnoresMouseEvents(native_view, ignore_mouse_events_);
}

}  // namespace electron::api
