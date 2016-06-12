// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"

namespace ui {
class MenuModel;
}

// A controller for the cross-platform menu model. The menu that's created
// has the tag and represented object set for each menu item. The object is a
// NSValue holding a pointer to the model for that level of the menu (to
// allow for hierarchical menus). The tag is the index into that model for
// that particular item. It is important that the model outlives this object
// as it only maintains weak references.
@interface AtomMenuController : NSObject<NSMenuDelegate> {
 @protected
  ui::MenuModel* model_;  // weak
  base::scoped_nsobject<NSMenu> menu_;
  BOOL isMenuOpen_;
}

@property(nonatomic, assign) ui::MenuModel* model;

// NIB-based initializer. This does not create a menu. Clients can set the
// properties of the object and the menu will be created upon the first call to
// |-menu|. Note that the menu will be immutable after creation.
- (id)init;

// Builds a NSMenu from the pre-built model (must not be nil). Changes made
// to the contents of the model after calling this will not be noticed.
- (id)initWithModel:(ui::MenuModel*)model;

// Populate current NSMenu with |model|.
- (void)populateWithModel:(ui::MenuModel*)model;

// Programmatically close the constructed menu.
- (void)cancel;

// Access to the constructed menu if the complex initializer was used. If the
// default initializer was used, then this will create the menu on first call.
- (NSMenu*)menu;

// Whether the menu is currently open.
- (BOOL)isMenuOpen;

// NSMenuDelegate methods this class implements. Subclasses should call super
// if extending the behavior.
- (void)menuWillOpen:(NSMenu*)menu;
- (void)menuDidClose:(NSMenu*)menu;

@end
