// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/ui/cocoa/electron_menu_controller.h"

#include <utility>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/mac/electron_application.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/platform_accelerator_cocoa.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/strings/grit/ui_strings.h"

using content::BrowserThread;

namespace {

struct Role {
  SEL selector;
  const char* role;
};
Role kRolesMap[] = {
    {@selector(orderFrontStandardAboutPanel:), "about"},
    {@selector(hide:), "hide"},
    {@selector(hideOtherApplications:), "hideothers"},
    {@selector(unhideAllApplications:), "unhide"},
    {@selector(arrangeInFront:), "front"},
    {@selector(undo:), "undo"},
    {@selector(redo:), "redo"},
    {@selector(cut:), "cut"},
    {@selector(copy:), "copy"},
    {@selector(paste:), "paste"},
    {@selector(delete:), "delete"},
    {@selector(pasteAndMatchStyle:), "pasteandmatchstyle"},
    {@selector(selectAll:), "selectall"},
    {@selector(startSpeaking:), "startspeaking"},
    {@selector(stopSpeaking:), "stopspeaking"},
    {@selector(performMiniaturize:), "minimize"},
    {@selector(performClose:), "close"},
    {@selector(performZoom:), "zoom"},
    {@selector(terminate:), "quit"},
    // ↓ is intentionally not `toggleFullScreen`. The macOS full screen menu
    // item behaves weird. If we use `toggleFullScreen`, then the menu item will
    // use the default label, and not take the one provided.
    {@selector(toggleFullScreenMode:), "togglefullscreen"},
    {@selector(toggleTabBar:), "toggletabbar"},
    {@selector(selectNextTab:), "selectnexttab"},
    {@selector(selectPreviousTab:), "selectprevioustab"},
    {@selector(mergeAllWindows:), "mergeallwindows"},
    {@selector(moveTabToNewWindow:), "movetabtonewwindow"},
    {@selector(clearRecentDocuments:), "clearrecentdocuments"},
};

// Called when adding a submenu to the menu and checks if the submenu, via its
// |model|, has visible child items.
bool MenuHasVisibleItems(const electron::ElectronMenuModel* model) {
  int count = model->GetItemCount();
  for (int index = 0; index < count; index++) {
    if (model->IsVisibleAt(index))
      return true;
  }
  return false;
}

// Called when an empty submenu is created. This inserts a menu item labeled
// "(empty)" into the submenu. Matches Windows behavior.
NSMenu* MakeEmptySubmenu() {
  base::scoped_nsobject<NSMenu> submenu([[NSMenu alloc] initWithTitle:@""]);
  NSString* empty_menu_title =
      l10n_util::GetNSString(IDS_APP_MENU_EMPTY_SUBMENU);

  [submenu addItemWithTitle:empty_menu_title action:NULL keyEquivalent:@""];
  [[submenu itemAtIndex:0] setEnabled:NO];
  return submenu.autorelease();
}

}  // namespace

// This class stores a base::WeakPtr<electron::ElectronMenuModel> as an
// Objective-C object, which allows it to be stored in the representedObject
// field of an NSMenuItem.
@interface WeakPtrToElectronMenuModelAsNSObject : NSObject
+ (instancetype)weakPtrForModel:(electron::ElectronMenuModel*)model;
+ (electron::ElectronMenuModel*)getFrom:(id)instance;
- (instancetype)initWithModel:(electron::ElectronMenuModel*)model;
- (electron::ElectronMenuModel*)menuModel;
@end

@implementation WeakPtrToElectronMenuModelAsNSObject {
  base::WeakPtr<electron::ElectronMenuModel> _model;
}

+ (instancetype)weakPtrForModel:(electron::ElectronMenuModel*)model {
  return [[[WeakPtrToElectronMenuModelAsNSObject alloc] initWithModel:model]
      autorelease];
}

+ (electron::ElectronMenuModel*)getFrom:(id)instance {
  return
      [base::mac::ObjCCastStrict<WeakPtrToElectronMenuModelAsNSObject>(instance)
          menuModel];
}

- (instancetype)initWithModel:(electron::ElectronMenuModel*)model {
  if ((self = [super init])) {
    _model = model->GetWeakPtr();
  }
  return self;
}

- (electron::ElectronMenuModel*)menuModel {
  return _model.get();
}

@end

// Menu item is located for ease of removing it from the parent owner
static base::scoped_nsobject<NSMenuItem> recentDocumentsMenuItem_;

// Submenu retained to be swapped back to |recentDocumentsMenuItem_|
static base::scoped_nsobject<NSMenu> recentDocumentsMenuSwap_;

@implementation ElectronMenuController

- (electron::ElectronMenuModel*)model {
  return model_.get();
}

- (void)setModel:(electron::ElectronMenuModel*)model {
  model_ = model->GetWeakPtr();
}

- (instancetype)initWithModel:(electron::ElectronMenuModel*)model
        useDefaultAccelerator:(BOOL)use {
  if ((self = [super init])) {
    model_ = model->GetWeakPtr();
    isMenuOpen_ = NO;
    useDefaultAccelerator_ = use;
    [self menu];
  }
  return self;
}

- (void)dealloc {
  [menu_ setDelegate:nil];

  // Close the menu if it is still open. This could happen if a tab gets closed
  // while its context menu is still open.
  [self cancel];

  model_ = nullptr;
  [super dealloc];
}

- (void)setCloseCallback:(base::OnceClosure)callback {
  closeCallback = std::move(callback);
}

- (void)populateWithModel:(electron::ElectronMenuModel*)model {
  if (!menu_)
    return;

  // Locate & retain the recent documents menu item
  if (!recentDocumentsMenuItem_) {
    base::string16 title = base::ASCIIToUTF16("Open Recent");
    NSString* openTitle = l10n_util::FixUpWindowsStyleLabel(title);

    recentDocumentsMenuItem_.reset([[[[[NSApp mainMenu]
        itemWithTitle:@"Electron"] submenu] itemWithTitle:openTitle] retain]);
  }

  model_ = model->GetWeakPtr();
  [menu_ removeAllItems];

  const int count = model->GetItemCount();
  for (int index = 0; index < count; index++) {
    if (model->GetTypeAt(index) == electron::ElectronMenuModel::TYPE_SEPARATOR)
      [self addSeparatorToMenu:menu_ atIndex:index];
    else
      [self addItemToMenu:menu_ atIndex:index fromModel:model];
  }
}

- (void)cancel {
  if (isMenuOpen_) {
    [menu_ cancelTracking];
    isMenuOpen_ = NO;
    if (model_)
      model_->MenuWillClose();
    if (!closeCallback.is_null()) {
      base::PostTask(FROM_HERE, {BrowserThread::UI}, std::move(closeCallback));
    }
  }
}

// Creates a NSMenu from the given model. If the model has submenus, this can
// be invoked recursively.
- (NSMenu*)menuFromModel:(electron::ElectronMenuModel*)model {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@""] autorelease];

  const int count = model->GetItemCount();
  for (int index = 0; index < count; index++) {
    if (model->GetTypeAt(index) == electron::ElectronMenuModel::TYPE_SEPARATOR)
      [self addSeparatorToMenu:menu atIndex:index];
    else
      [self addItemToMenu:menu atIndex:index fromModel:model];
  }

  return menu;
}

// Adds a separator item at the given index. As the separator doesn't need
// anything from the model, this method doesn't need the model index as the
// other method below does.
- (void)addSeparatorToMenu:(NSMenu*)menu atIndex:(int)index {
  NSMenuItem* separator = [NSMenuItem separatorItem];
  [menu insertItem:separator atIndex:index];
}

// Empties the source menu items to the destination.
- (void)moveMenuItems:(NSMenu*)source to:(NSMenu*)destination {
  const NSInteger count = [source numberOfItems];
  for (NSInteger index = 0; index < count; index++) {
    NSMenuItem* removedItem = [[[source itemAtIndex:0] retain] autorelease];
    [source removeItemAtIndex:0];
    [destination addItem:removedItem];
  }
}

// Replaces the item's submenu instance with the singleton recent documents
// menu. Previously replaced menu items will be recovered.
- (void)replaceSubmenuShowingRecentDocuments:(NSMenuItem*)item {
  NSMenu* recentDocumentsMenu =
      [[[recentDocumentsMenuItem_ submenu] retain] autorelease];

  // Remove menu items in recent documents back to swap menu
  [self moveMenuItems:recentDocumentsMenu to:recentDocumentsMenuSwap_];
  // Swap back the submenu
  [recentDocumentsMenuItem_ setSubmenu:recentDocumentsMenuSwap_];

  // Retain the item's submenu for a future recovery
  recentDocumentsMenuSwap_.reset([[item submenu] retain]);

  // Repopulate with items from the submenu to be replaced
  [self moveMenuItems:recentDocumentsMenuSwap_ to:recentDocumentsMenu];
  // Update the submenu's title
  [recentDocumentsMenu setTitle:[recentDocumentsMenuSwap_ title]];
  // Replace submenu
  [item setSubmenu:recentDocumentsMenu];

  DCHECK_EQ([item action], @selector(submenuAction:));
  DCHECK_EQ([item target], recentDocumentsMenu);

  // Remember the new menu item that carries the recent documents menu
  recentDocumentsMenuItem_.reset([item retain]);
}

// Adds an item or a hierarchical menu to the item at the |index|,
// associated with the entry in the model identified by |modelIndex|.
- (void)addItemToMenu:(NSMenu*)menu
              atIndex:(NSInteger)index
            fromModel:(electron::ElectronMenuModel*)model {
  base::string16 label16 = model->GetLabelAt(index);
  NSString* label = l10n_util::FixUpWindowsStyleLabel(label16);

  base::scoped_nsobject<NSMenuItem> item([[NSMenuItem alloc]
      initWithTitle:label
             action:@selector(itemSelected:)
      keyEquivalent:@""]);

  // If the menu item has an icon, set it.
  ui::ImageModel icon = model->GetIconAt(index);
  if (icon.IsImage())
    [item setImage:icon.GetImage().ToNSImage()];

  base::string16 toolTip = model->GetToolTipAt(index);
  [item setToolTip:base::SysUTF16ToNSString(toolTip)];

  base::string16 role = model->GetRoleAt(index);
  electron::ElectronMenuModel::ItemType type = model->GetTypeAt(index);

  if (role == base::ASCIIToUTF16("services")) {
    base::string16 title = base::ASCIIToUTF16("Services");
    NSString* label = l10n_util::FixUpWindowsStyleLabel(title);

    [item setTarget:nil];
    [item setAction:nil];
    NSMenu* submenu = [[NSMenu alloc] initWithTitle:label];
    [item setSubmenu:submenu];
    [NSApp setServicesMenu:submenu];
  } else if (type == electron::ElectronMenuModel::TYPE_SUBMENU &&
             model->IsVisibleAt(index)) {
    // We need to specifically check that the submenu top-level item has been
    // enabled as it's not validated by validateUserInterfaceItem
    if (!model->IsEnabledAt(index))
      [item setEnabled:NO];

    // Recursively build a submenu from the sub-model at this index.
    [item setTarget:nil];
    [item setAction:nil];
    electron::ElectronMenuModel* submenuModel =
        static_cast<electron::ElectronMenuModel*>(
            model->GetSubmenuModelAt(index));
    NSMenu* submenu = MenuHasVisibleItems(submenuModel)
                          ? [self menuFromModel:submenuModel]
                          : MakeEmptySubmenu();
    [submenu setTitle:[item title]];
    [item setSubmenu:submenu];

    // Set submenu's role.
    if ((role == base::ASCIIToUTF16("window") ||
         role == base::ASCIIToUTF16("windowmenu")) &&
        [submenu numberOfItems])
      [NSApp setWindowsMenu:submenu];
    else if (role == base::ASCIIToUTF16("help"))
      [NSApp setHelpMenu:submenu];
    else if (role == base::ASCIIToUTF16("recentdocuments"))
      [self replaceSubmenuShowingRecentDocuments:item];
  } else {
    // The MenuModel works on indexes so we can't just set the command id as the
    // tag like we do in other menus. Also set the represented object to be
    // the model so hierarchical menus check the correct index in the correct
    // model. Setting the target to |self| allows this class to participate
    // in validation of the menu items.
    [item setTag:index];
    [item setRepresentedObject:[WeakPtrToElectronMenuModelAsNSObject
                                   weakPtrForModel:model]];
    ui::Accelerator accelerator;
    if (model->GetAcceleratorAtWithParams(index, useDefaultAccelerator_,
                                          &accelerator)) {
      NSString* key_equivalent;
      NSUInteger modifier_mask;
      GetKeyEquivalentAndModifierMaskFromAccelerator(
          accelerator, &key_equivalent, &modifier_mask);
      [item setKeyEquivalent:key_equivalent];
      [item setKeyEquivalentModifierMask:modifier_mask];
    }

    if (@available(macOS 10.13, *)) {
      [(id)item
          setAllowsKeyEquivalentWhenHidden:(model->WorksWhenHiddenAt(index))];
    }

    // Set menu item's role.
    [item setTarget:self];
    if (!role.empty()) {
      for (const Role& pair : kRolesMap) {
        if (role == base::ASCIIToUTF16(pair.role)) {
          [item setTarget:nil];
          [item setAction:pair.selector];
          break;
        }
      }
    }
  }
  [menu insertItem:item atIndex:index];
}

// Called before the menu is to be displayed to update the state (enabled,
// radio, etc) of each item in the menu.
- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  SEL action = [item action];
  if (action != @selector(itemSelected:))
    return NO;

  NSInteger modelIndex = [item tag];
  electron::ElectronMenuModel* model = [WeakPtrToElectronMenuModelAsNSObject
      getFrom:[(id)item representedObject]];
  DCHECK(model);
  if (model) {
    BOOL checked = model->IsItemCheckedAt(modelIndex);
    DCHECK([(id)item isKindOfClass:[NSMenuItem class]]);

    [(id)item setState:(checked ? NSOnState : NSOffState)];
    [(id)item setHidden:(!model->IsVisibleAt(modelIndex))];

    return model->IsEnabledAt(modelIndex);
  }
  return NO;
}

// Called when the user chooses a particular menu item. |sender| is the menu
// item chosen.
- (void)itemSelected:(id)sender {
  NSInteger modelIndex = [sender tag];
  electron::ElectronMenuModel* model =
      [WeakPtrToElectronMenuModelAsNSObject getFrom:[sender representedObject]];
  DCHECK(model);
  if (model) {
    NSEvent* event = [NSApp currentEvent];
    model->ActivatedAt(modelIndex, ui::EventFlagsFromNSEventWithModifiers(
                                       event, [event modifierFlags]));
  }
}

- (NSMenu*)menu {
  if (menu_)
    return menu_.get();

  menu_.reset([[NSMenu alloc] initWithTitle:@""]);
  [menu_ setDelegate:self];
  if (model_)
    [self populateWithModel:model_.get()];
  return menu_.get();
}

- (BOOL)isMenuOpen {
  return isMenuOpen_;
}

- (void)menuWillOpen:(NSMenu*)menu {
  isMenuOpen_ = YES;
  if (model_)
    model_->MenuWillShow();
}

- (void)menuDidClose:(NSMenu*)menu {
  if (isMenuOpen_) {
    isMenuOpen_ = NO;
    if (model_)
      model_->MenuWillClose();
    // Post async task so that itemSelected runs before the close callback
    // deletes the controller from the map which deallocates it
    if (!closeCallback.is_null()) {
      base::PostTask(FROM_HERE, {BrowserThread::UI}, std::move(closeCallback));
    }
  }
}

@end
