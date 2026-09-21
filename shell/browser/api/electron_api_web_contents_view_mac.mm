// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_contents_view.h"

#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>

#include "base/no_destructor.h"
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

  Class subclass = objc_allocateClassPair(base_class, subclass_name.c_str(), 0);
  if (!subclass)
    return nil;

  // Match the real -hitTest: type encoding rather than hardcoding one.
  Method hit_test = class_getInstanceMethod(base_class, @selector(hitTest:));
  class_addMethod(subclass, @selector(hitTest:),
                  reinterpret_cast<IMP>(NonInteractiveHitTest),
                  method_getTypeEncoding(hit_test));

  // -class must keep reporting the original class so any code doing class
  // checks, and KVO's own isa games, don't observe our substitution.
  // This mirrors what KVO does for its dynamic subclasses.
  class_addMethod(subclass, @selector(class),
                  imp_implementationWithBlock(^Class(id _self) {
                    return base_class;
                  }),
                  "#@:");

  objc_registerClassPair(subclass);
  return subclass;
}

// Tracks each view's true class so we can restore it exactly.
std::map<NSView*, Class>& OriginalClasses() {
  static base::NoDestructor<std::map<NSView*, Class>> instance;
  return *instance;
}

void SetViewHitTestable(NSView* view, bool hit_testable) {
  auto& originals = OriginalClasses();
  auto it = originals.find(view);
  const bool currently_swizzled = it != originals.end();

  if (hit_testable) {
    if (!currently_swizzled)
      return;
    object_setClass(view, it->second);
    originals.erase(it);
    return;
  }

  if (currently_swizzled)
    return;

  // object_getClass(), not -class: we need the real isa, which may already be
  // a KVO subclass we must subclass in turn rather than clobber.
  Class original = object_getClass(view);
  if (Class subclass = GetOrCreateNonInteractiveSubclass(original)) {
    object_setClass(view, subclass);
    originals[view] = original;
  }
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