// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/app_shim/app_shim_application.h"

#include "shell/app_shim/app_shim_controller.h"

namespace {

// A screen reader sets AXEnhancedUserInterface as it starts, and other apps
// set and clear it in passing; only one that keeps it set gets web content.
constexpr NSTimeInterval kCompleteModeDelay = 2.0;

void EnableAccessibilitySupport(
    electron::mojom::AppShimAccessibilityMode mode) {
  if (auto* controller = electron::AppShimController::Get())
    controller->EnableAccessibilitySupport(mode);
}

}  // namespace

@implementation ElectronAppShimApplication

- (void)enableCompleteAccessibility {
  EnableAccessibilitySupport(
      electron::mojom::AppShimAccessibilityMode::kComplete);
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString*)attribute {
  // AXEnhancedUserInterface is what screen readers set; AXManualAccessibility
  // is what Electron documents for other apps to ask for accessibility.
  const bool is_manual = [attribute isEqualToString:@"AXManualAccessibility"];
  if (is_manual || [attribute isEqualToString:@"AXEnhancedUserInterface"]) {
    [NSObject
        cancelPreviousPerformRequestsWithTarget:self
                                       selector:@selector(
                                                    enableCompleteAccessibility)
                                         object:nil];
    if ([value boolValue]) {
      [self performSelector:@selector(enableCompleteAccessibility)
                 withObject:nil
                 afterDelay:is_manual ? 0 : kCompleteModeDelay];
    }
    if (is_manual)
      return;
  }
  [super accessibilitySetValue:value forAttribute:attribute];
}

- (NSArray*)accessibilityAttributeNames {
  return [[super accessibilityAttributeNames]
      arrayByAddingObject:@"AXManualAccessibility"];
}

- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute {
  return [attribute isEqualToString:@"AXManualAccessibility"] ||
         [super accessibilityIsAttributeSettable:attribute];
}

- (NSAccessibilityRole)accessibilityRole {
  // Something is reading this app's accessibility tree.
  EnableAccessibilitySupport(
      electron::mojom::AppShimAccessibilityMode::kPartial);
  return [super accessibilityRole];
}

@end
