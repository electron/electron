// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/ui/cocoa/atom_menu_controller.h"

#include "atom/browser/ui/atom_menu_model.h"
#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/platform_accelerator_cocoa.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/gfx/image/image.h"

@interface AtomMenuController (Private)
- (void)addSeparatorToMenu:(NSMenu*)menu
                   atIndex:(int)index;
@end

@implementation AtomMenuController

@synthesize model = model_;

- (id)init {
  if ((self = [super init]))
    [self menu];
  return self;
}

- (id)initWithModel:(ui::MenuModel*)model {
  if ((self = [super init])) {
    model_ = model;
    [self menu];
  }
  return self;
}

- (void)dealloc {
  [menu_ setDelegate:nil];

  // Close the menu if it is still open. This could happen if a tab gets closed
  // while its context menu is still open.
  [self cancel];

  model_ = NULL;
  [super dealloc];
}

- (void)populateWithModel:(ui::MenuModel*)model {
  if (!menu_)
    return;

  model_ = model;
  [menu_ removeAllItems];

  const int count = model->GetItemCount();
  for (int index = 0; index < count; index++) {
    if (model->GetTypeAt(index) == ui::MenuModel::TYPE_SEPARATOR)
      [self addSeparatorToMenu:menu_ atIndex:index];
    else
      [self addItemToMenu:menu_ atIndex:index fromModel:model];
  }
}

- (void)cancel {
  if (isMenuOpen_) {
    [menu_ cancelTracking];
    model_->MenuClosed();
    isMenuOpen_ = NO;
  }
}

// Creates a NSMenu from the given model. If the model has submenus, this can
// be invoked recursively.
- (NSMenu*)menuFromModel:(ui::MenuModel*)model {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@""] autorelease];

  const int count = model->GetItemCount();
  for (int index = 0; index < count; index++) {
    if (model->GetTypeAt(index) == ui::MenuModel::TYPE_SEPARATOR)
      [self addSeparatorToMenu:menu atIndex:index];
    else
      [self addItemToMenu:menu atIndex:index fromModel:model];
  }

  return menu;
}

// Adds a separator item at the given index. As the separator doesn't need
// anything from the model, this method doesn't need the model index as the
// other method below does.
- (void)addSeparatorToMenu:(NSMenu*)menu
                   atIndex:(int)index {
  NSMenuItem* separator = [NSMenuItem separatorItem];
  [menu insertItem:separator atIndex:index];
}

// Adds an item or a hierarchical menu to the item at the |index|,
// associated with the entry in the model identified by |modelIndex|.
- (void)addItemToMenu:(NSMenu*)menu
              atIndex:(NSInteger)index
            fromModel:(ui::MenuModel*)model {
  base::string16 label16 = model->GetLabelAt(index);
  NSString* label = l10n_util::FixUpWindowsStyleLabel(label16);
  base::scoped_nsobject<NSMenuItem> item(
      [[NSMenuItem alloc] initWithTitle:label
                                 action:@selector(itemSelected:)
                          keyEquivalent:@""]);

  // If the menu item has an icon, set it.
  gfx::Image icon;
  if (model->GetIconAt(index, &icon) && !icon.IsEmpty())
    [item setImage:icon.ToNSImage()];

  ui::MenuModel::ItemType type = model->GetTypeAt(index);
  if (type == ui::MenuModel::TYPE_SUBMENU) {
    // Recursively build a submenu from the sub-model at this index.
    [item setTarget:nil];
    [item setAction:nil];
    ui::MenuModel* submenuModel = model->GetSubmenuModelAt(index);
    NSMenu* submenu = [self menuFromModel:submenuModel];
    [submenu setTitle:[item title]];
    [item setSubmenu:submenu];

    // Hack to set window and help menu.
    if ([[item title] isEqualToString:@"Window"] && [submenu numberOfItems] > 0)
      [NSApp setWindowsMenu:submenu];
    else if ([[item title] isEqualToString:@"Help"])
      [NSApp setHelpMenu:submenu];
    if ([[item title] isEqualToString:@"Services"] &&
        [submenu numberOfItems] == 0)
      [NSApp setServicesMenu:submenu];
  } else {
    // The MenuModel works on indexes so we can't just set the command id as the
    // tag like we do in other menus. Also set the represented object to be
    // the model so hierarchical menus check the correct index in the correct
    // model. Setting the target to |self| allows this class to participate
    // in validation of the menu items.
    [item setTag:index];
    [item setTarget:self];
    NSValue* modelObject = [NSValue valueWithPointer:model];
    [item setRepresentedObject:modelObject];  // Retains |modelObject|.
    ui::Accelerator accelerator;
    if (model->GetAcceleratorAt(index, &accelerator)) {
      const ui::PlatformAcceleratorCocoa* platformAccelerator =
          static_cast<const ui::PlatformAcceleratorCocoa*>(
              accelerator.platform_accelerator());
      if (platformAccelerator) {
        [item setKeyEquivalent:platformAccelerator->characters()];
        [item setKeyEquivalentModifierMask:
            platformAccelerator->modifier_mask()];
      }
    }
  }
  [menu insertItem:item atIndex:index];
}

// Called before the menu is to be displayed to update the state (enabled,
// radio, etc) of each item in the menu. Also will update the title if
// the item is marked as "dynamic".
- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  SEL action = [item action];
  if (action != @selector(itemSelected:))
    return NO;

  NSInteger modelIndex = [item tag];
  ui::MenuModel* model =
      static_cast<ui::MenuModel*>(
          [[(id)item representedObject] pointerValue]);
  DCHECK(model);
  if (model) {
    BOOL checked = model->IsItemCheckedAt(modelIndex);
    DCHECK([(id)item isKindOfClass:[NSMenuItem class]]);
    [(id)item setState:(checked ? NSOnState : NSOffState)];
    [(id)item setHidden:(!model->IsVisibleAt(modelIndex))];
    if (model->IsItemDynamicAt(modelIndex)) {
      // Update the label and the icon.
      NSString* label =
          l10n_util::FixUpWindowsStyleLabel(model->GetLabelAt(modelIndex));
      [(id)item setTitle:label];

      gfx::Image icon;
      model->GetIconAt(modelIndex, &icon);
      [(id)item setImage:icon.IsEmpty() ? nil : icon.ToNSImage()];
    }
    return model->IsEnabledAt(modelIndex);
  }
  return NO;
}

// Called when the user chooses a particular menu item. |sender| is the menu
// item chosen.
- (void)itemSelected:(id)sender {
  NSInteger modelIndex = [sender tag];
  ui::MenuModel* model =
      static_cast<ui::MenuModel*>(
          [[sender representedObject] pointerValue]);
  DCHECK(model);
  if (model) {
    NSEvent* event = [NSApp currentEvent];
    model->ActivatedAt(modelIndex,
                       ui::EventFlagsFromModifiers([event modifierFlags]));
  }
}

- (NSMenu*)menu {
  if (menu_)
    return menu_.get();

  menu_.reset([[NSMenu alloc] initWithTitle:@""]);
  [menu_ setDelegate:self];
  if (model_)
    [self populateWithModel:model_];
  return menu_.get();
}

- (BOOL)isMenuOpen {
  return isMenuOpen_;
}

- (void)menuWillOpen:(NSMenu*)menu {
  isMenuOpen_ = YES;
  model_->MenuWillShow();
}

- (void)menuDidClose:(NSMenu*)menu {
  if (isMenuOpen_) {
    model_->MenuClosed();
    isMenuOpen_ = NO;
  }
}

@end
