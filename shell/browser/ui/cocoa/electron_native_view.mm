// Copyright (c) 2022 GitHub, Inc.

#include "shell/browser/ui/cocoa/electron_native_view.h"

#include <objc/objc-runtime.h>

#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "shell/browser/native_window_mac.h"
#include "shell/browser/ui/cocoa/electron_ns_window.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"

typedef struct CGContext* CGContextRef;

@implementation ElectronNativeView

- (void)dealloc {
  if ([self shell])
    [self shell]->NotifyViewIsDeleting();
  [super dealloc];
}

- (electron::NativeViewPrivate*)nativeViewPrivate {
  return &private_;
}

- (void)setNativeBackgroundColor:(SkColor)color {
  background_color_ = absl::make_optional(color);
  [self setNeedsDisplay:YES];
}

- (BOOL)isFlipped {
  return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
  if (!background_color_.has_value())
    return;

  SkColor background_color = background_color_.value();
  gfx::RectF dirty(dirtyRect);
  CGContextRef context = reinterpret_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);
  CGContextSetRGBStrokeColor(context, SkColorGetR(background_color) / 255.0f,
                             SkColorGetG(background_color) / 255.0f,
                             SkColorGetB(background_color) / 255.0f,
                             SkColorGetA(background_color) / 255.0f);
  CGContextSetRGBFillColor(context, SkColorGetR(background_color) / 255.0f,
                           SkColorGetG(background_color) / 255.0f,
                           SkColorGetB(background_color) / 255.0f,
                           SkColorGetA(background_color) / 255.0f);
  CGContextFillRect(context, dirty.ToCGRect());
}

@end

namespace electron {

namespace {

bool IsFramelessWindow(NSView* view) {
  if (![view window])
    return false;
  if (![[view window] respondsToSelector:@selector(shell)])
    return false;
  return ![static_cast<ElectronNSWindow*>([view window]) shell]->has_frame();
}

bool NativeViewInjected(NSView* self, SEL _cmd) {
  return true;
}

NativeView* GetShell(NSView* self, SEL _cmd) {
  return [self nativeViewPrivate]->shell;
}

// This method is directly called by NSWindow during a window resize on OSX
// 10.10.0, beta 2. We must override it to prevent the content view from
// shrinking.
void SetFrameSize(NSView* self, SEL _cmd, NSSize size) {
  if (IsFramelessWindow(self) && [self nativeViewPrivate]->is_content_view &&
      [self superview])
    size = [[self superview] bounds].size;

  auto super_impl = reinterpret_cast<decltype(&SetFrameSize)>(
      [[self superclass] instanceMethodForSelector:_cmd]);
  super_impl(self, _cmd, size);
}

void ViewDidMoveToSuperview(NSView* self, SEL _cmd) {
  if (!IsFramelessWindow(self) || ![self nativeViewPrivate]->is_content_view) {
    auto super_impl = reinterpret_cast<decltype(&ViewDidMoveToSuperview)>(
        [[self superclass] instanceMethodForSelector:_cmd]);
    super_impl(self, _cmd);
    return;
  }

  [self setFrame:[[self superview] bounds]];
}

}  // namespace

NativeViewPrivate::NativeViewPrivate() {}

NativeViewPrivate::~NativeViewPrivate() {}

bool IsNativeView(id view) {
  return [view respondsToSelector:@selector(nativeViewPrivate)];
}

bool NativeViewMethodsInstalled(Class cl) {
  return class_getClassMethod(cl, @selector(nativeViewInjected)) != nullptr;
}

void InstallNativeViewMethods(Class cl) {
  class_addMethod(cl, @selector(nativeViewInjected), (IMP)NativeViewInjected,
                  "B@:");
  class_addMethod(cl, @selector(shell), (IMP)GetShell, "^v@:");
  class_addMethod(cl, @selector(setFrameSize:), (IMP)SetFrameSize,
                  "v@:{_NSSize=ff}");
  class_addMethod(cl, @selector(viewDidMoveToSuperview),
                  (IMP)ViewDidMoveToSuperview, "v@:");
}

}  // namespace electron
