// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/ui/cocoa/electron_menu_controller.h"

#include <string>
#include <utility>

#include "base/apple/foundation_util.h"
#include "base/functional/callback.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/apple/url_conversions.h"
#include "shell/browser/mac/electron_application.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/window_list.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/events/keycodes/keyboard_code_conversion_mac.h"
#include "ui/strings/grit/ui_strings.h"

using content::BrowserThread;
using SharingItem = electron::ElectronMenuModel::SharingItem;

namespace {

static NSMenuItem* __strong recentDocumentsMenuItem_;
static NSMenu* __strong recentDocumentsMenuSwap_;

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
    {@selector(orderFrontSubstitutionsPanel:), "showsubstitutions"},
    {@selector(toggleAutomaticQuoteSubstitution:), "togglesmartquotes"},
    {@selector(toggleAutomaticDashSubstitution:), "togglesmartdashes"},
    {@selector(toggleAutomaticTextReplacement:), "toggletextreplacement"},
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
    {@selector(toggleTabOverview:), "showalltabs"},
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
  NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
  submenu.autoenablesItems = NO;
  NSString* empty_menu_title =
      l10n_util::GetNSString(IDS_APP_MENU_EMPTY_SUBMENU);

  [submenu addItemWithTitle:empty_menu_title action:nullptr keyEquivalent:@""];
  [[submenu itemAtIndex:0] setEnabled:NO];
  return submenu;
}

// Convert an SharingItem to an array of NSObjects.
NSArray* ConvertSharingItemToNS(const SharingItem& item) {
  NSMutableArray* result = [NSMutableArray array];
  if (item.texts) {
    for (const std::string& str : *item.texts)
      [result addObject:base::SysUTF8ToNSString(str)];
  }
  if (item.file_paths) {
    for (const base::FilePath& path : *item.file_paths)
      [result addObject:base::apple::FilePathToNSURL(path)];
  }
  if (item.urls) {
    for (const GURL& url : *item.urls)
      [result addObject:net::NSURLWithGURL(url)];
  }
  return result;
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
  return [[WeakPtrToElectronMenuModelAsNSObject alloc] initWithModel:model];
}

+ (electron::ElectronMenuModel*)getFrom:(id)instance {
  return [base::apple::ObjCCastStrict<WeakPtrToElectronMenuModelAsNSObject>(
      instance) menuModel];
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
}

- (void)setPopupCloseCallback:(base::OnceClosure)callback {
  popupCloseCallback = std::move(callback);
}

- (void)populateWithModel:(electron::ElectronMenuModel*)model {
  if (!menu_)
    return;

  // Locate & retain the recent documents menu item
  if (!recentDocumentsMenuItem_) {
    std::u16string title = u"Open Recent";
    NSString* openTitle = l10n_util::FixUpWindowsStyleLabel(title);

    recentDocumentsMenuItem_ = [[[[NSApp mainMenu] itemWithTitle:@"Electron"]
        submenu] itemWithTitle:openTitle];
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
    if (!popupCloseCallback.is_null()) {
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE, std::move(popupCloseCallback));
    }
  }
}

// Creates a NSMenu from the given model. If the model has submenus, this can
// be invoked recursively.
- (NSMenu*)menuFromModel:(electron::ElectronMenuModel*)model {
  NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];
  // We manually manage enabled/disabled/hidden state for every item,
  // including Cocoa role-based selectors.
  menu.autoenablesItems = NO;

  const int count = model->GetItemCount();
  for (int index = 0; index < count; index++) {
    if (model->GetTypeAt(index) == electron::ElectronMenuModel::TYPE_SEPARATOR)
      [self addSeparatorToMenu:menu atIndex:index];
    else
      [self addItemToMenu:menu atIndex:index fromModel:model];
  }

  menu.delegate = self;
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
    NSMenuItem* removedItem = [source itemAtIndex:0];
    [source removeItemAtIndex:0];
    [destination addItem:removedItem];
  }
}

// Replaces the item's submenu instance with the singleton recent documents
// menu. Previously replaced menu items will be recovered.
- (void)replaceSubmenuShowingRecentDocuments:(NSMenuItem*)item {
  NSMenu* recentDocumentsMenu = [recentDocumentsMenuItem_ submenu];

  // Remove menu items in recent documents back to swap menu
  [self moveMenuItems:recentDocumentsMenu to:recentDocumentsMenuSwap_];
  // Swap back the submenu
  [recentDocumentsMenuItem_ setSubmenu:recentDocumentsMenuSwap_];

  // Retain the item's submenu for a future recovery
  recentDocumentsMenuSwap_ = [item submenu];

  // Repopulate with items from the submenu to be replaced
  [self moveMenuItems:recentDocumentsMenuSwap_ to:recentDocumentsMenu];
  // Update the submenu's title
  [recentDocumentsMenu setTitle:[recentDocumentsMenuSwap_ title]];
  // Replace submenu
  [item setSubmenu:recentDocumentsMenu];

  DCHECK_EQ([item action], @selector(submenuAction:));
  DCHECK_EQ([item target], recentDocumentsMenu);

  // Remember the new menu item that carries the recent documents menu
  recentDocumentsMenuItem_ = item;
}

// Fill the menu with Share Menu items.
- (NSMenu*)createShareMenuForItem:(const SharingItem&)item {
  NSArray* items = ConvertSharingItemToNS(item);
  if ([items count] == 0)
    return MakeEmptySubmenu();
  NSMenu* menu = [[NSMenu alloc] init];
  menu.autoenablesItems = NO;
  NSArray* services = [NSSharingService sharingServicesForItems:items];
  for (NSSharingService* service in services)
    [menu addItem:[self menuItemForService:service withItems:items]];
  [menu setDelegate:self];
  return menu;
}

// Creates a menu item that calls |service| when invoked.
- (NSMenuItem*)menuItemForService:(NSSharingService*)service
                        withItems:(NSArray*)items {
  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:service.menuItemTitle
                                                action:@selector(performShare:)
                                         keyEquivalent:@""];
  [item setTarget:self];
  [item setImage:service.image];
  [item setRepresentedObject:@{@"service" : service, @"items" : items}];
  return item;
}

- (NSMenuItem*)makeMenuItemForIndex:(NSInteger)index
                          fromModel:(electron::ElectronMenuModel*)model {
  std::u16string label16 = model->GetLabelAt(index);
  auto rawSecondaryLabel = model->GetSecondaryLabelAt(index);
  NSString* label = l10n_util::FixUpWindowsStyleLabel(label16);

  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                action:@selector(itemSelected:)
                                         keyEquivalent:@""];

  if (!rawSecondaryLabel.empty()) {
    if (@available(macOS 14.4, *)) {
      NSString* secondary_label =
          l10n_util::FixUpWindowsStyleLabel(rawSecondaryLabel);
      item.subtitle = secondary_label;
    }
  }

  std::u16string role = model->GetRoleAt(index);
  electron::ElectronMenuModel::ItemType type = model->GetTypeAt(index);
  std::u16string customType = model->GetCustomTypeAt(index);

  // The sectionHeaderWithTitle menu item is only available in macOS 14.0+.
  if (@available(macOS 14, *)) {
    if (customType == u"header") {
      item = [NSMenuItem sectionHeaderWithTitle:label];
    }
  }

  // If the menu item has an icon, set it.
  ui::ImageModel icon = model->GetIconAt(index);
  if (icon.IsImage())
    [item setImage:icon.GetImage().ToNSImage()];

  std::u16string toolTip = model->GetToolTipAt(index);
  [item setToolTip:base::SysUTF16ToNSString(toolTip)];

  if (role == u"services") {
    std::u16string title = u"Services";
    NSString* sub_label = l10n_util::FixUpWindowsStyleLabel(title);

    item.target = nil;
    item.action = nil;
    NSMenu* submenu = [[NSMenu alloc] initWithTitle:sub_label];
    item.submenu = submenu;
    [NSApp setServicesMenu:submenu];
  } else if (role == u"sharemenu") {
    SharingItem sharing_item;
    model->GetSharingItemAt(index, &sharing_item);
    item.target = nil;
    item.action = nil;
    [item setSubmenu:[self createShareMenuForItem:sharing_item]];
  } else if (type == electron::ElectronMenuModel::TYPE_SUBMENU &&
             model->IsVisibleAt(index)) {
    // Recursively build a submenu from the sub-model at this index.
    item.target = nil;
    item.action = nil;
    electron::ElectronMenuModel* submenuModel =
        static_cast<electron::ElectronMenuModel*>(
            model->GetSubmenuModelAt(index));
    NSMenu* submenu = MenuHasVisibleItems(submenuModel)
                          ? [self menuFromModel:submenuModel]
                          : MakeEmptySubmenu();

    // NSMenuPresentationStylePalette is only available in macOS 14.0+.
    if (@available(macOS 14, *)) {
      if (customType == u"palette") {
        submenu.presentationStyle = NSMenuPresentationStylePalette;
      }
    }

    submenu.title = item.title;
    item.submenu = submenu;
    item.tag = index;
    item.representedObject =
        [WeakPtrToElectronMenuModelAsNSObject weakPtrForModel:model];
    submenu.delegate = self;

    // Set submenu's role.
    if ((role == u"window" || role == u"windowmenu") && [submenu numberOfItems])
      [NSApp setWindowsMenu:submenu];
    else if (role == u"help")
      [NSApp setHelpMenu:submenu];
    else if (role == u"recentdocuments")
      [self replaceSubmenuShowingRecentDocuments:item];
  } else {
    // The MenuModel works on indexes so we can't just set the command id as the
    // tag like we do in other menus. Also set the represented object to be
    // the model so hierarchical menus check the correct index in the correct
    // model. Setting the target to |self| allows this class to participate
    // in validation of the menu items.
    item.tag = index;
    item.representedObject =
        [WeakPtrToElectronMenuModelAsNSObject weakPtrForModel:model];
    ui::Accelerator accelerator;
    if (model->GetAcceleratorAtWithParams(index, useDefaultAccelerator_,
                                          &accelerator)) {
      // Note that we are not using Chromium's
      // GetKeyEquivalentAndModifierMaskFromAccelerator API,
      // because it will convert Shift+Character to ShiftedCharacter, for
      // example Shift+/ would be converted to ?, which is against macOS HIG.
      // See also https://github.com/electron/electron/issues/21790.
      NSUInteger modifier_mask = 0;
      if (accelerator.IsCtrlDown())
        modifier_mask |= NSEventModifierFlagControl;
      if (accelerator.IsAltDown())
        modifier_mask |= NSEventModifierFlagOption;
      if (accelerator.IsCmdDown())
        modifier_mask |= NSEventModifierFlagCommand;
      unichar character;
      if (accelerator.shifted_char) {
        // When a shifted char is explicitly specified, for example Ctrl+Plus,
        // use the shifted char directly.
        character = static_cast<unichar>(*accelerator.shifted_char);
      } else {
        // Otherwise use the unshifted combinations, for example Ctrl+Shift+=.
        if (accelerator.IsShiftDown())
          modifier_mask |= NSEventModifierFlagShift;
        ui::MacKeyCodeForWindowsKeyCode(accelerator.key_code(), modifier_mask,
                                        nullptr, &character);
      }
      item.keyEquivalent = [NSString stringWithFormat:@"%C", character];
      item.keyEquivalentModifierMask = modifier_mask;
    }

    [(id)item
        setAllowsKeyEquivalentWhenHidden:(model->WorksWhenHiddenAt(index))];

    // Set menu item's role.
    item.target = self;
    if (!role.empty()) {
      for (const Role& pair : kRolesMap) {
        if (role == base::ASCIIToUTF16(pair.role)) {
          item.target = nil;
          item.action = pair.selector;
          break;
        }
      }
    }
  }

  return item;
}

- (void)applyStateToMenuItem:(NSMenuItem*)item {
  id represented = item.representedObject;
  if (!represented)
    return;

  if (![represented
          isKindOfClass:[WeakPtrToElectronMenuModelAsNSObject class]]) {
    NSLog(@"representedObject is not a WeakPtrToElectronMenuModelAsNSObject");
    return;
  }

  electron::ElectronMenuModel* model =
      [WeakPtrToElectronMenuModelAsNSObject getFrom:represented];
  if (!model)
    return;

  NSInteger index = item.tag;
  int count = model->GetItemCount();
  if (index < 0 || index >= count)
    return;

  // When the menu is closed, we need to allow shortcuts to be triggered even
  // if the menu item is disabled. So we only disable the menu item when the
  // menu is open. This matches behavior of |validateUserInterfaceItem|.
  item.enabled = model->IsEnabledAt(index) || !isMenuOpen_;
  item.hidden = !model->IsVisibleAt(index);
  item.state = model->IsItemCheckedAt(index) ? NSControlStateValueOn
                                             : NSControlStateValueOff;
}

- (void)refreshMenuTree:(NSMenu*)menu {
  for (NSMenuItem* item in menu.itemArray) {
    [self applyStateToMenuItem:item];
    if (item.submenu)
      [self refreshMenuTree:item.submenu];
  }
}

// Adds an item or a hierarchical menu to the item at the |index|,
// associated with the entry in the model identified by |modelIndex|.
- (void)addItemToMenu:(NSMenu*)menu
              atIndex:(NSInteger)index
            fromModel:(electron::ElectronMenuModel*)model {
  [menu insertItem:[self makeMenuItemForIndex:index fromModel:model]
           atIndex:index];
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

// Performs the share action using the sharing service represented by |sender|.
- (void)performShare:(NSMenuItem*)sender {
  NSDictionary* object =
      base::apple::ObjCCastStrict<NSDictionary>([sender representedObject]);
  NSSharingService* service =
      base::apple::ObjCCastStrict<NSSharingService>(object[@"service"]);
  NSArray* items = base::apple::ObjCCastStrict<NSArray>(object[@"items"]);
  [service setDelegate:self];
  [service performWithItems:items];
}

- (NSMenu*)menu {
  if (menu_)
    return menu_;

  if (model_ && model_->sharing_item()) {
    NSMenu* menu = [self createShareMenuForItem:*model_->sharing_item()];
    menu_ = menu;
  } else {
    menu_ = [[NSMenu alloc] initWithTitle:@""];
    menu_.autoenablesItems = NO;
    if (model_)
      [self populateWithModel:model_.get()];
  }
  menu_.delegate = self;
  return menu_;
}

- (BOOL)isMenuOpen {
  return isMenuOpen_;
}

- (void)menuWillOpen:(NSMenu*)menu {
  isMenuOpen_ = YES;

  // macOS automatically injects a duplicate "Toggle Full Screen" menu item
  // when we set menu.delegate on submenus. Remove hidden duplicates.
  for (NSMenuItem* item in menu.itemArray) {
    if (item.isHidden && item.action == @selector(toggleFullScreenMode:))
      [menu removeItem:item];
  }

  [self refreshMenuTree:menu];
  if (model_)
    model_->MenuWillShow();
}

- (void)menuDidClose:(NSMenu*)menu {
  // If the menu is already closed, do nothing.
  if (!isMenuOpen_)
    return;

  isMenuOpen_ = NO;
  [self refreshMenuTree:menu];

  // There are two scenarios where we should emit menu-did-close:
  // 1. It's a popup and the top level menu is closed.
  // 2. It's an application menu, and the current menu's supermenu
  //    is the top-level menu.
  bool has_close_cb = !popupCloseCallback.is_null();
  if (menu != menu_) {
    if (has_close_cb || menu.supermenu != menu_)
      return;
  }

  if (model_)
    model_->MenuWillClose();
  // Post async task so that itemSelected runs before the close callback
  // deletes the controller from the map which deallocates it.
  if (has_close_cb) {
    content::GetUIThreadTaskRunner({})->PostTask(FROM_HERE,
                                                 std::move(popupCloseCallback));
  }
}

// NSSharingServiceDelegate

- (NSWindow*)sharingService:(NSSharingService*)service
    sourceWindowForShareItems:(NSArray*)items
          sharingContentScope:(NSSharingContentScope*)scope {
  // Return the current active window.
  const auto& list = electron::WindowList::GetWindows();
  for (electron::NativeWindow* window : list) {
    if (window->IsFocused())
      return window->GetNativeWindow().GetNativeNSWindow();
  }
  return nil;
}

@end
