// Copyright (c) 2022 GitHub, Inc.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"

namespace electron {

class NativeView;

// A private class that holds views specific private data.
// Object-C does not support multi-inheiritance, so it is impossible to add
// common data members for UI elements.
struct NativeViewPrivate {
  NativeViewPrivate();
  ~NativeViewPrivate();

  NativeView* shell = nullptr;
  bool is_content_view = false;
  bool wants_layer = false;
  bool wants_layer_infected = false;
};

}  // namespace electron

// The methods that every View should implemented.
@protocol ElectronNativeViewProtocol
- (electron::NativeViewPrivate*)nativeViewPrivate;
- (void)setNativeBackgroundColor:(SkColor)color;
@end

@interface ElectronNativeView : NSView <ElectronNativeViewProtocol> {
 @private
  electron::NativeViewPrivate private_;
  absl::optional<SkColor> background_color_;
}
@end

// Extended methods of ElectronNativeViewProtocol.
@interface NSView (ElectronNativeViewMethods) <ElectronNativeViewProtocol>
- (electron::NativeView*)shell;
@end

namespace electron {

// Return whether a class is part of views system.
bool IsNativeView(id view);

// Return whether a class has been installed with custom methods.
bool NativeViewMethodsInstalled(Class cl);

// Add custom view methods to class.
void InstallNativeViewMethods(Class cl);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_VIEW_H_
