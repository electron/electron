// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_cocoa.h"

#include <string>
#include <vector>

#include "base/message_loop/message_loop_current.h"
#include "base/message_loop/message_pump_mac.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/ui/cocoa/NSString+ANSI.h"
#include "shell/browser/ui/cocoa/atom_menu_controller.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/gfx/mac/coordinate_conversion.h"
#include "ui/native_theme/native_theme.h"

@interface StatusItemView : NSView {
  electron::TrayIconCocoa* trayIcon_;   // weak
  AtomMenuController* menuController_;  // weak
  BOOL ignoreDoubleClickEvents_;
  base::scoped_nsobject<NSStatusItem> statusItem_;
  base::scoped_nsobject<NSTrackingArea> trackingArea_;
}

@end  // @interface StatusItemView

@implementation StatusItemView

- (void)dealloc {
  trayIcon_ = nil;
  menuController_ = nil;
  [super dealloc];
}

- (id)initWithIcon:(electron::TrayIconCocoa*)icon {
  trayIcon_ = icon;
  menuController_ = nil;
  ignoreDoubleClickEvents_ = NO;

  if ((self = [super initWithFrame:CGRectZero])) {
    [self registerForDraggedTypes:@[
      NSFilenamesPboardType,
      NSStringPboardType,
    ]];

    // Create the status item.
    NSStatusItem* item = [[NSStatusBar systemStatusBar]
        statusItemWithLength:NSVariableStatusItemLength];
    statusItem_.reset([item retain]);
    [[statusItem_ button] addSubview:self];  // inject custom view
    [self updateDimensions];
  }
  return self;
}

- (void)updateDimensions {
  [self setFrame:[statusItem_ button].frame];
}

- (void)updateTrackingAreas {
  // Use NSTrackingArea for listening to mouseEnter, mouseExit, and mouseMove
  // events.
  [self removeTrackingArea:trackingArea_];
  trackingArea_.reset([[NSTrackingArea alloc]
      initWithRect:[self bounds]
           options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                   NSTrackingActiveAlways
             owner:self
          userInfo:nil]);
  [self addTrackingArea:trackingArea_];
}

- (void)removeItem {
  // Turn off tracking events to prevent crash.
  if (trackingArea_) {
    [self removeTrackingArea:trackingArea_];
    trackingArea_.reset();
  }
  [[NSStatusBar systemStatusBar] removeStatusItem:statusItem_];
  [self removeFromSuperview];
  statusItem_.reset();
}

- (void)setImage:(NSImage*)image {
  [[statusItem_ button] setImage:[image copy]];
  [self updateDimensions];
}

- (void)setAlternateImage:(NSImage*)image {
  [[statusItem_ button] setAlternateImage:[image copy]];
}

- (void)setIgnoreDoubleClickEvents:(BOOL)ignore {
  ignoreDoubleClickEvents_ = ignore;
}

- (BOOL)getIgnoreDoubleClickEvents {
  return ignoreDoubleClickEvents_;
}

- (void)setTitle:(NSString*)title {
  if ([title containsANSICodes]) {
    [[statusItem_ button]
        setAttributedTitle:[title attributedStringParsingANSICodes]];
  } else {
    [[statusItem_ button] setTitle:[title copy]];
  }

  // Fix icon margins.
  if (title.length > 0) {
    [[statusItem_ button] setImagePosition:NSImageLeft];
  } else {
    [[statusItem_ button] setImagePosition:NSImageOnly];
  }

  [self updateDimensions];
}

- (NSString*)title {
  return [statusItem_ button].title;
}

- (void)setMenuController:(AtomMenuController*)menu {
  menuController_ = menu;
  [statusItem_ setMenu:[menuController_ menu]];
}

- (void)mouseDown:(NSEvent*)event {
  // Pass click to superclass to show menu. Custom mouseUp handler won't be
  // invoked.
  if (menuController_) {
    [super mouseDown:event];
  } else {
    [[statusItem_ button] highlight:YES];
  }
}

- (void)mouseUp:(NSEvent*)event {
  [[statusItem_ button] highlight:NO];

  // If we are ignoring double click events, we should ignore the `clickCount`
  // value and immediately emit a click event.
  BOOL shouldBeHandledAsASingleClick =
      (event.clickCount == 1) || ignoreDoubleClickEvents_;
  if (shouldBeHandledAsASingleClick)
    trayIcon_->NotifyClicked(
        gfx::ScreenRectFromNSRect(event.window.frame),
        gfx::ScreenPointFromNSPoint([event locationInWindow]),
        ui::EventFlagsFromModifiers([event modifierFlags]));

  // Double click event.
  BOOL shouldBeHandledAsADoubleClick =
      (event.clickCount == 2) && !ignoreDoubleClickEvents_;
  if (shouldBeHandledAsADoubleClick)
    trayIcon_->NotifyDoubleClicked(
        gfx::ScreenRectFromNSRect(event.window.frame),
        ui::EventFlagsFromModifiers([event modifierFlags]));
}

- (void)popUpContextMenu:(electron::AtomMenuModel*)menu_model {
  // Make sure events can be pumped while the menu is up.
  base::MessageLoopCurrent::ScopedNestableTaskAllower allow;

  // Show a custom menu.
  if (menu_model) {
    base::scoped_nsobject<AtomMenuController> menuController(
        [[AtomMenuController alloc] initWithModel:menu_model
                            useDefaultAccelerator:NO]);
    // Hacky way to mimic design of ordinary tray menu.
    [statusItem_ setMenu:[menuController menu]];
    [[statusItem_ button] performClick:self];
    [statusItem_ setMenu:[menuController_ menu]];
    return;
  }

  if (menuController_ && ![menuController_ isMenuOpen]) {
    // Ensure the UI can update while the menu is fading out.
    base::ScopedPumpMessagesInPrivateModes pump_private;

    [[statusItem_ button] performClick:self];
  }
}

- (void)rightMouseUp:(NSEvent*)event {
  trayIcon_->NotifyRightClicked(
      gfx::ScreenRectFromNSRect(event.window.frame),
      ui::EventFlagsFromModifiers([event modifierFlags]));
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  trayIcon_->NotifyDragEntered();
  return NSDragOperationCopy;
}

- (void)mouseExited:(NSEvent*)event {
  trayIcon_->NotifyMouseExited(
      gfx::ScreenPointFromNSPoint([event locationInWindow]),
      ui::EventFlagsFromModifiers([event modifierFlags]));
}

- (void)mouseEntered:(NSEvent*)event {
  trayIcon_->NotifyMouseEntered(
      gfx::ScreenPointFromNSPoint([event locationInWindow]),
      ui::EventFlagsFromModifiers([event modifierFlags]));
}

- (void)mouseMoved:(NSEvent*)event {
  trayIcon_->NotifyMouseMoved(
      gfx::ScreenPointFromNSPoint([event locationInWindow]),
      ui::EventFlagsFromModifiers([event modifierFlags]));
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
  trayIcon_->NotifyDragExited();
}

- (void)draggingEnded:(id<NSDraggingInfo>)sender {
  trayIcon_->NotifyDragEnded();

  if (NSPointInRect([sender draggingLocation], self.frame)) {
    trayIcon_->NotifyDrop();
  }
}

- (BOOL)handleDrop:(id<NSDraggingInfo>)sender {
  NSPasteboard* pboard = [sender draggingPasteboard];

  if ([[pboard types] containsObject:NSFilenamesPboardType]) {
    std::vector<std::string> dropFiles;
    NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
    for (NSString* file in files)
      dropFiles.push_back(base::SysNSStringToUTF8(file));
    trayIcon_->NotifyDropFiles(dropFiles);
    return YES;
  } else if ([[pboard types] containsObject:NSStringPboardType]) {
    NSString* dropText = [pboard stringForType:NSStringPboardType];
    trayIcon_->NotifyDropText(base::SysNSStringToUTF8(dropText));
    return YES;
  }

  return NO;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender {
  return YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  [self handleDrop:sender];
  return YES;
}

@end

namespace electron {

TrayIconCocoa::TrayIconCocoa() : weak_factory_(this) {
  status_item_view_.reset([[StatusItemView alloc] initWithIcon:this]);
}

TrayIconCocoa::~TrayIconCocoa() {
  [status_item_view_ removeItem];
}

void TrayIconCocoa::SetImage(const gfx::Image& image) {
  [status_item_view_ setImage:image.AsNSImage()];
}

void TrayIconCocoa::SetPressedImage(const gfx::Image& image) {
  [status_item_view_ setAlternateImage:image.AsNSImage()];
}

void TrayIconCocoa::SetToolTip(const std::string& tool_tip) {
  [status_item_view_ setToolTip:base::SysUTF8ToNSString(tool_tip)];
}

void TrayIconCocoa::SetTitle(const std::string& title) {
  [status_item_view_ setTitle:base::SysUTF8ToNSString(title)];
}

std::string TrayIconCocoa::GetTitle() {
  return base::SysNSStringToUTF8([status_item_view_ title]);
}

void TrayIconCocoa::SetIgnoreDoubleClickEvents(bool ignore) {
  [status_item_view_ setIgnoreDoubleClickEvents:ignore];
}

bool TrayIconCocoa::GetIgnoreDoubleClickEvents() {
  return [status_item_view_ getIgnoreDoubleClickEvents];
}

void TrayIconCocoa::PopUpOnUI(AtomMenuModel* menu_model) {
  [status_item_view_ popUpContextMenu:menu_model];
}

void TrayIconCocoa::PopUpContextMenu(const gfx::Point& pos,
                                     AtomMenuModel* menu_model) {
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&TrayIconCocoa::PopUpOnUI, weak_factory_.GetWeakPtr(),
                     base::Unretained(menu_model)));
}

void TrayIconCocoa::SetContextMenu(AtomMenuModel* menu_model) {
  if (menu_model) {
    // Create native menu.
    menu_.reset([[AtomMenuController alloc] initWithModel:menu_model
                                    useDefaultAccelerator:NO]);
  } else {
    menu_.reset();
  }
  [status_item_view_ setMenuController:menu_.get()];
}

gfx::Rect TrayIconCocoa::GetBounds() {
  return gfx::ScreenRectFromNSRect([status_item_view_ window].frame);
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconCocoa;
}

}  // namespace electron
