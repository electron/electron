// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_cocoa.h"

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/message_loop/message_pump_apple.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/current_thread.h"
#include "base/uuid.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/ui/cocoa/NSString+ANSI.h"
#include "shell/browser/ui/cocoa/electron_menu_controller.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/gfx/mac/coordinate_conversion.h"

@interface StatusItemView : NSView {
  raw_ptr<electron::TrayIconCocoa> trayIcon_;  // weak
  ElectronMenuController* menuController_;     // weak
  BOOL ignoreDoubleClickEvents_;
  NSStatusItem* __strong statusItem_;
  NSTrackingArea* __strong trackingArea_;
}

@end  // @interface StatusItemView

@implementation StatusItemView

- (void)dealloc {
  trayIcon_ = nil;
  menuController_ = nil;
}

- (id)initWithIcon:(electron::TrayIconCocoa*)icon {
  trayIcon_ = icon;
  menuController_ = nil;
  ignoreDoubleClickEvents_ = NO;

  if ((self = [super initWithFrame:CGRectZero])) {
    [self registerForDraggedTypes:@[
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      NSFilenamesPboardType,
#pragma clang diagnostic pop
      NSPasteboardTypeString,
    ]];

    // Create the status item.
    NSStatusItem* item = [[NSStatusBar systemStatusBar]
        statusItemWithLength:NSVariableStatusItemLength];
    statusItem_ = item;
    [[statusItem_ button] addSubview:self];

    // We need to set the target and action on the button, otherwise
    // VoiceOver doesn't know where to send the select action.
    [[statusItem_ button] setTarget:self];
    [[statusItem_ button] setAction:@selector(mouseDown:)];
    [self updateDimensions];
  }
  return self;
}

- (void)updateDimensions {
  [self setFrame:[statusItem_ button].frame];
}

- (void)setAutosaveName:(NSString*)name {
  statusItem_.autosaveName = name;
}

- (void)updateTrackingAreas {
  // Use NSTrackingArea for listening to mouseEnter, mouseExit, and mouseMove
  // events.
  [self removeTrackingArea:trackingArea_];
  trackingArea_ = [[NSTrackingArea alloc]
      initWithRect:[self bounds]
           options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                   NSTrackingActiveAlways
             owner:self
          userInfo:nil];
  [self addTrackingArea:trackingArea_];
}

- (void)removeItem {
  // Turn off tracking events to prevent crash.
  if (trackingArea_) {
    [self removeTrackingArea:trackingArea_];
    trackingArea_ = nil;
  }

  // Ensure any open menu is closed.
  if ([statusItem_ menu])
    [[statusItem_ menu] cancelTracking];

  [[NSStatusBar systemStatusBar] removeStatusItem:statusItem_];
  [self removeFromSuperview];
  statusItem_ = nil;
}

- (void)setImage:(NSImage*)image {
  [[statusItem_ button] setImage:image];
  [self updateDimensions];
}

- (void)setAlternateImage:(NSImage*)image {
  [[statusItem_ button] setAlternateImage:image];

  // We need to change the button type here because the default button type for
  // NSStatusItem, NSStatusBarButton, does not display alternate content when
  // clicked. NSButtonTypeMomentaryChange displays its alternate content when
  // clicked and returns to its normal content when the user releases it, which
  // is the behavior users would expect when clicking a button with an alternate
  // image set.
  [[statusItem_ button] setButtonType:NSButtonTypeMomentaryChange];
  [self updateDimensions];
}

- (void)setIgnoreDoubleClickEvents:(BOOL)ignore {
  ignoreDoubleClickEvents_ = ignore;
}

- (BOOL)getIgnoreDoubleClickEvents {
  return ignoreDoubleClickEvents_;
}

- (void)setTitle:(NSString*)title font_type:(NSString*)font_type {
  NSMutableAttributedString* attributed_title =
      [[NSMutableAttributedString alloc] initWithString:title];

  if ([title containsANSICodes]) {
    attributed_title = [title attributedStringParsingANSICodes];
  }

  // Change font type, if specified
  CGFloat existing_size = [[[statusItem_ button] font] pointSize];
  if ([font_type isEqualToString:@"monospaced"]) {
    NSDictionary* attributes = @{
      NSFontAttributeName :
          [NSFont monospacedSystemFontOfSize:existing_size
                                      weight:NSFontWeightRegular]
    };
    [attributed_title addAttributes:attributes
                              range:NSMakeRange(0, [attributed_title length])];
  } else if ([font_type isEqualToString:@"monospacedDigit"]) {
    NSDictionary* attributes = @{
      NSFontAttributeName :
          [NSFont monospacedDigitSystemFontOfSize:existing_size
                                           weight:NSFontWeightRegular]
    };
    [attributed_title addAttributes:attributes
                              range:NSMakeRange(0, [attributed_title length])];
  }

  // Set title
  [[statusItem_ button] setAttributedTitle:attributed_title];

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

- (void)setMenuController:(ElectronMenuController*)menu {
  menuController_ = menu;
  [statusItem_ setMenu:[menuController_ menu]];
}

- (void)handleClickNotifications:(NSEvent*)event {
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

- (void)mouseDown:(NSEvent*)event {
  // If |event| does not respond to locationInWindow, we've
  // arrived here from VoiceOver, which does not pass an event.
  // Create a synthetic event to pass to the click handler.
  if (![event respondsToSelector:@selector(locationInWindow)]) {
    event = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown
                               location:NSMakePoint(0, 0)
                          modifierFlags:0
                              timestamp:NSApp.currentEvent.timestamp
                           windowNumber:0
                                context:nil
                            eventNumber:0
                             clickCount:1
                               pressure:1.0];

    // We also need to explicitly call the click handler here, since
    // VoiceOver won't trigger mouseUp.
    [self handleClickNotifications:event];
  }

  trayIcon_->NotifyMouseDown(
      gfx::ScreenPointFromNSPoint([event locationInWindow]),
      ui::EventFlagsFromModifiers([event modifierFlags]));

  // Pass click to superclass to show menu if one exists and has a non-zero
  // number of items. Custom mouseUp handler won't be invoked in this case.
  if (menuController_ && [[menuController_ menu] numberOfItems] > 0) {
    [self handleClickNotifications:event];
    [super mouseDown:event];
  } else {
    [[statusItem_ button] highlight:YES];
  }
}

- (void)mouseUp:(NSEvent*)event {
  [[statusItem_ button] highlight:NO];

  trayIcon_->NotifyMouseUp(
      gfx::ScreenPointFromNSPoint([event locationInWindow]),
      ui::EventFlagsFromModifiers([event modifierFlags]));

  [self handleClickNotifications:event];
}

- (void)popUpContextMenu:(electron::ElectronMenuModel*)menu_model {
  // Make sure events can be pumped while the menu is up.
  base::CurrentThread::ScopedAllowApplicationTasksInNativeNestedLoop allow;

  // Show a custom menu.
  if (menu_model) {
    ElectronMenuController* menuController =
        [[ElectronMenuController alloc] initWithModel:menu_model
                                useDefaultAccelerator:NO];
    // Hacky way to mimic design of ordinary tray menu.
    [statusItem_ setMenu:[menuController menu]];
    base::WeakPtr<electron::TrayIconCocoa> weak_tray_icon =
        trayIcon_->GetWeakPtr();
    [[statusItem_ button] performClick:self];
    // /⚠️ \ Warning! Arbitrary JavaScript and who knows what else has been run
    // during -performClick:. This object may have been deleted.
    // We check if |trayIcon_| is still alive as it owns us and has the same
    // lifetime.
    if (!weak_tray_icon)
      return;
    [statusItem_ setMenu:[menuController_ menu]];
    return;
  }

  if (menuController_ && ![menuController_ isMenuOpen]) {
    // Ensure the UI can update while the menu is fading out.
    base::ScopedPumpMessagesInPrivateModes pump_private;
    [[statusItem_ button] performClick:self];
  }
}

- (void)closeContextMenu {
  if (menuController_) {
    [menuController_ cancel];
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

// TODO(codebytere): update to currently supported NSPasteboardTypeFileURL or
// kUTTypeFileURL.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  if ([[pboard types] containsObject:NSFilenamesPboardType]) {
    std::vector<std::string> dropFiles;
    NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
    for (NSString* file in files)
      dropFiles.push_back(base::SysNSStringToUTF8(file));
    trayIcon_->NotifyDropFiles(dropFiles);
    return YES;
  } else if ([[pboard types] containsObject:NSPasteboardTypeString]) {
    NSString* dropText = [pboard stringForType:NSPasteboardTypeString];
    trayIcon_->NotifyDropText(base::SysNSStringToUTF8(dropText));
    return YES;
  }
#pragma clang diagnostic pop
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

TrayIconCocoa::TrayIconCocoa() {
  status_item_view_ = [[StatusItemView alloc] initWithIcon:this];
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

void TrayIconCocoa::SetTitle(const std::string& title,
                             const TitleOptions& options) {
  [status_item_view_ setTitle:base::SysUTF8ToNSString(title)
                    font_type:base::SysUTF8ToNSString(options.font_type)];
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

void TrayIconCocoa::PopUpOnUI(base::WeakPtr<ElectronMenuModel> menu_model) {
  [status_item_view_ popUpContextMenu:menu_model.get()];
}

void TrayIconCocoa::PopUpContextMenu(
    const gfx::Point& pos,
    base::WeakPtr<ElectronMenuModel> menu_model) {
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&TrayIconCocoa::PopUpOnUI,
                                weak_factory_.GetWeakPtr(), menu_model));
}

void TrayIconCocoa::CloseContextMenu() {
  [status_item_view_ closeContextMenu];
}

void TrayIconCocoa::SetContextMenu(raw_ptr<ElectronMenuModel> menu_model) {
  if (menu_model) {
    // Create native menu.
    menu_ = [[ElectronMenuController alloc] initWithModel:menu_model
                                    useDefaultAccelerator:NO];
  } else {
    menu_ = nil;
  }
  [status_item_view_ setMenuController:menu_];
}

gfx::Rect TrayIconCocoa::GetBounds() {
  return gfx::ScreenRectFromNSRect([status_item_view_ window].frame);
}

void TrayIconCocoa::SetAutoSaveName(const std::string& name) {
  [status_item_view_ setAutosaveName:base::SysUTF8ToNSString(name)];
}

// static
TrayIcon* TrayIcon::Create(std::optional<base::Uuid> guid) {
  return new TrayIconCocoa;
}

}  // namespace electron
