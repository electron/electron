// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_contents_view.h"

#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace {

// Suffix appended to a view's real class name to form our synthesized
// non-interactive subclass, e.g.
// "RenderWidgetHostViewCocoa_ElectronNonInteractive".
constexpr char kSubclassSuffix[] = "_ElectronNonInteractive";

// -hitTest: override installed on the synthesized subclass only. Returning nil
// is AppKit's "this point isn't mine" signal: the superview's default
// implementation keeps walking its remaining subviews, so the click falls
// through to whatever sibling is underneath — the Cocoa analogue of
// aura's EventTargetingPolicy::kNone.
NSView* NonInteractiveHitTest(id self_view, SEL _cmd, NSPoint point) {
  return nil;
}

// Returns (creating on first use) the non-interactive subclass of `base_class`.
// Cached per base class, since registering the same name twice fails.
Class GetOrCreateNonInteractiveSubclass(Class base_class) {
  const std::string subclass_name =
      base::StringPrintf("%s%s", class_getName(base_class), kSubclassSuffix);

  if (Class existing = objc_lookUpClass(subclass_name.c_str()))
    return existing;

  // Look this up before allocating the class pair so that bailing out on a
  // missing method doesn't leave an allocated-but-unregistered class behind.
  Method hit_test = class_getInstanceMethod(base_class, @selector(hitTest:));
  if (!hit_test)
    return nil;

  Class subclass = objc_allocateClassPair(base_class, subclass_name.c_str(), 0);
  if (!subclass)
    return nil;

  class_addMethod(subclass, @selector(hitTest:),
                  reinterpret_cast<IMP>(NonInteractiveHitTest),
                  method_getTypeEncoding(hit_test));

  class_addMethod(subclass, @selector(class),
                  imp_implementationWithBlock(^Class(id _self) {
                    return base_class;
                  }),
                  "#@:");

  objc_registerClassPair(subclass);
  return subclass;
}

bool IsNonInteractive(NSView* view) {
  const char* name = class_getName(object_getClass(view));
  return base::EndsWith(name,
                        kSubclassSuffix);  // from base/strings/string_util.h
}

void SetViewHitTestable(NSView* view, bool hit_testable) {
  Class current = object_getClass(view);
  const bool currently_swizzled = IsNonInteractive(view);

  if (hit_testable) {
    if (currently_swizzled) {
      if (Class superclass = class_getSuperclass(current))
        object_setClass(view, superclass);
    }
    return;
  }

  if (currently_swizzled)
    return;

  if (Class subclass = GetOrCreateNonInteractiveSubclass(current))
    object_setClass(view, subclass);
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

  SetViewHitTestable(content_view, GetInteractive());
}

}  // namespace electron::api
