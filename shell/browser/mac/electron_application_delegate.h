// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "shell/browser/ui/cocoa/electron_menu_controller.h"

@interface ElectronApplicationDelegate : NSObject <NSApplicationDelegate> {
 @private
  base::scoped_nsobject<ElectronMenuController> menu_controller_;
}

// Sets the menu that will be returned in "applicationDockMenu:".
- (void)setApplicationDockMenu:(electron::ElectronMenuModel*)model;

@end
