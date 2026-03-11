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

// A transparent container view that doesn't intercept mouse events.
// This allows the status bar button to receive all mouse events while
// we overlay custom image layers on top of its placeholder image.
// Without this, the image views would intercept clicks and prevent
// the button from receiving events.
@interface TransparentLayerContainer : NSView
@end

@implementation TransparentLayerContainer
- (NSView*)hitTest:(NSPoint)point {
  // Always return nil so mouse events pass through to views below
  return nil;
}
@end

@interface StatusItemView : NSView {
  raw_ptr<electron::TrayIconCocoa> trayIcon_;  // weak
  ElectronMenuController* menuController_;     // weak
  BOOL ignoreDoubleClickEvents_;
  NSStatusItem* __strong statusItem_;
  NSTrackingArea* __strong trackingArea_;
  NSView* __strong layerContainer_;
  NSView* __strong alternateLayerContainer_;
  BOOL menuIsOpen_;
  NSMenu* __weak currentMenu_;
}

@end  // @interface StatusItemView

@implementation StatusItemView

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  trayIcon_ = nil;
  menuController_ = nil;
}

- (id)initWithIcon:(electron::TrayIconCocoa*)icon {
  trayIcon_ = icon;
  menuController_ = nil;
  ignoreDoubleClickEvents_ = NO;
  layerContainer_ = nil;
  alternateLayerContainer_ = nil;
  menuIsOpen_ = NO;
  currentMenu_ = nil;

  if ((self = [super initWithFrame:CGRectZero])) {
    [self registerForDraggedTypes:@[
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      NSFilenamesPboardType,
#pragma clang diagnostic pop
      NSPasteboardTypeString,
    ]];

    // Create the status item.
    statusItem_ = [[NSStatusBar systemStatusBar]
        statusItemWithLength:NSVariableStatusItemLength];

    // Install ourselves as a subview of the button.
    NSStatusBarButton* btn = [statusItem_ button];
    self.translatesAutoresizingMaskIntoConstraints = YES;
    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [btn addSubview:self];

    // Set target and action for VoiceOver support.
    [btn setTarget:self];
    [btn setAction:@selector(mouseDown:)];
    [self updateDimensions];
  }
  return self;
}

// Synchronizes this view's frame with the status bar button's bounds and
// requests a layout pass. Call this after any change that might affect the
// button's size (e.g., setting images, changing title, changing image
// position).
- (void)updateDimensions {
  [self setFrame:[statusItem_ button].bounds];
  [self setNeedsLayout:YES];
}

- (void)setAutosaveName:(NSString*)name {
  statusItem_.autosaveName = name;
}

// Helper method to register menu notifications for tracking menu open/close
// state. This enables proper layer visibility toggling when menus are
// displayed.
- (void)registerMenuNotifications:(NSMenu*)menu {
  if (!menu)
    return;

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(menuDidBeginTracking:)
             name:NSMenuDidBeginTrackingNotification
           object:menu];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(menuDidEndTracking:)
             name:NSMenuDidEndTrackingNotification
           object:menu];
}

// Helper method to unregister menu notifications for a specific menu.
// Call this before replacing a menu to prevent observer leaks.
- (void)unregisterMenuNotifications:(NSMenu*)menu {
  if (!menu)
    return;

  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSMenuDidBeginTrackingNotification
              object:menu];

  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSMenuDidEndTrackingNotification
              object:menu];
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

// Removes and clears the normal (non-pressed) layer container.
// Call this before setting a new image or updating layers.
- (void)clearLayerContainer {
  [layerContainer_ removeFromSuperview];
  layerContainer_ = nil;
}

// Removes and clears the alternate (pressed) layer container.
// Call this before setting a new pressed image or updating pressed layers.
- (void)clearAlternateLayerContainer {
  [alternateLayerContainer_ removeFromSuperview];
  alternateLayerContainer_ = nil;
}

- (void)setImage:(NSImage*)image {
  [self clearLayerContainer];
  [[statusItem_ button] setImage:image];
  [self updateDimensions];
}

// Calculates the maximum size needed to contain all layers.
// Used to determine the placeholder image size for the status bar button.
- (NSSize)maxSizeForLayers:(NSArray<NSImage*>*)layers {
  NSSize maxSize = NSZeroSize;
  for (NSImage* layer in layers) {
    maxSize.width = MAX(maxSize.width, layer.size.width);
    maxSize.height = MAX(maxSize.height, layer.size.height);
  }
  return maxSize;
}

// Creates a configured NSImageView for displaying a single layer image.
// The image view is centered, non-scaling, and non-interactive.
- (NSImageView*)createLayerImageView:(NSImage*)image {
  NSImageView* imageView = [[NSImageView alloc] initWithFrame:NSZeroRect];
  imageView.image = image;
  imageView.imageScaling = NSImageScaleNone;
  imageView.imageAlignment = NSImageAlignCenter;
  imageView.imageFrameStyle = NSImageFrameNone;
  imageView.editable = NO;
  imageView.animates = NO;

  return imageView;
}

// Creates a container view with all layer images stacked on top of each other.
// The container uses TransparentLayerContainer to pass mouse events through.
// Returns nil if layers is empty.
- (NSView*)createLayerContainerWithImages:(NSArray<NSImage*>*)layers
                                   hidden:(BOOL)hidden {
  if (layers.count == 0)
    return nil;

  // Use transparent container that doesn't intercept mouse events
  NSView* container =
      [[TransparentLayerContainer alloc] initWithFrame:NSZeroRect];
  container.hidden = hidden;

  for (NSImage* layer in layers) {
    NSImageView* imageView = [self createLayerImageView:layer];
    [container addSubview:imageView];
  }

  return container;
}

// Sets layered images for the normal (non-pressed) tray icon state.
// Multiple images are stacked on top of each other (index 0 = bottom layer).
// Creates a transparent placeholder on the button to reserve space, then
// overlays the actual images in a container that passes mouse events through.
- (void)setLayeredImages:(NSArray<NSImage*>*)layers {
  [self clearLayerContainer];

  if (layers.count == 0) {
    [[statusItem_ button] setImage:nil];
    [self updateDimensions];
    return;
  }

  NSSize maxSize = [self maxSizeForLayers:layers];
  if (maxSize.width <= 0 || maxSize.height <= 0) {
    return;
  }

  // Create a transparent placeholder so the button reserves space.
  [[statusItem_ button] setImage:[[NSImage alloc] initWithSize:maxSize]];

  // Hide normal layers if menu is currently open and we have alternate layers.
  NSStatusBarButton* btn = [statusItem_ button];
  BOOL hasAlternateImage = [btn alternateImage] != nil;
  BOOL shouldHide = menuIsOpen_ && hasAlternateImage;
  layerContainer_ = [self createLayerContainerWithImages:layers
                                                  hidden:shouldHide];
  [self addSubview:layerContainer_];
  [self updateDimensions];
}

// Sets layered images for the alternate (pressed/menu-open) tray icon state.
// Multiple images are stacked on top of each other (index 0 = bottom layer).
// This is shown when the button is pressed or when a menu is open.
- (void)setAlternateLayeredImages:(NSArray<NSImage*>*)layers {
  [self clearAlternateLayerContainer];

  if (layers.count == 0) {
    [[statusItem_ button] setAlternateImage:nil];
    [[statusItem_ button] setButtonType:NSButtonTypeMomentaryPushIn];
    [self updateDimensions];
    return;
  }

  NSSize maxSize = [self maxSizeForLayers:layers];
  if (maxSize.width <= 0 || maxSize.height <= 0) {
    return;
  }

  // Create a transparent placeholder for alternate image.
  [[statusItem_ button]
      setAlternateImage:[[NSImage alloc] initWithSize:maxSize]];
  [[statusItem_ button] setButtonType:NSButtonTypeMomentaryChange];

  // Show alternate layers if menu is currently open.
  BOOL shouldShow = menuIsOpen_;
  alternateLayerContainer_ = [self createLayerContainerWithImages:layers
                                                           hidden:!shouldShow];
  [self addSubview:alternateLayerContainer_];
  [self updateDimensions];
}

- (void)layout {
  [super layout];

  NSStatusBarButton* btn = [statusItem_ button];

  // Get the exact rect where the button draws images using its cell.
  NSRect imageRect = NSZeroRect;
  if ([btn respondsToSelector:@selector(cell)]) {
    NSCell* cell = [btn cell];
    if ([cell respondsToSelector:@selector(imageRectForBounds:)]) {
      imageRect = [cell imageRectForBounds:btn.bounds];
    }
  }

  if (NSIsEmptyRect(imageRect)) {
    imageRect = self.bounds;  // fallback
  }

  // Position containers to match where the button draws images.
  if (layerContainer_) {
    layerContainer_.frame = imageRect;
    // Each imageView fills its container; imageAlignment centers the image.
    for (NSView* subview in layerContainer_.subviews) {
      subview.frame = layerContainer_.bounds;
    }
  }

  if (alternateLayerContainer_) {
    alternateLayerContainer_.frame = imageRect;
    // Each imageView fills its container; imageAlignment centers the image.
    for (NSView* subview in alternateLayerContainer_.subviews) {
      subview.frame = alternateLayerContainer_.bounds;
    }
  }
}

- (void)setAlternateImage:(NSImage*)image {
  [self clearAlternateLayerContainer];

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

// Updates the visibility of normal and alternate layer containers based on
// the current menu state and button highlight state. This ensures the correct
// layers are shown when the button is pressed or when a menu is open.
- (void)updateLayerVisibility {
  NSStatusBarButton* btn = [statusItem_ button];
  BOOL hasAlternateImage = [btn alternateImage] != nil;

  // Show alternate layers when menu is open OR button is highlighted (pressed)
  BOOL showAlternate = menuIsOpen_ || [[btn cell] isHighlighted];

  alternateLayerContainer_.hidden = !showAlternate;
  // Only hide normal layers if we have alternate layers to show
  layerContainer_.hidden = showAlternate && hasAlternateImage;

  // Ensure layout updates to reflect visibility changes
  [self setNeedsLayout:YES];
}

- (void)setMenuController:(ElectronMenuController*)menu {
  menuController_ = menu;
  NSMenu* nsMenu = [menuController_ menu];

  // Unregister from previous menu to prevent observer leaks.
  [self unregisterMenuNotifications:currentMenu_];

  // Register for new menu notifications.
  [self registerMenuNotifications:nsMenu];
  currentMenu_ = nsMenu;

  [statusItem_ setMenu:nsMenu];
}

- (void)menuDidBeginTracking:(NSNotification*)notification {
  menuIsOpen_ = YES;
  [self updateLayerVisibility];
}

- (void)menuDidEndTracking:(NSNotification*)notification {
  menuIsOpen_ = NO;
  [self updateLayerVisibility];
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
    // Note: menu notifications handle layer visibility for menu case
  } else {
    // No menu - manually highlight button and update layer visibility
    [[statusItem_ button] highlight:YES];
    [self updateLayerVisibility];
  }
}

- (void)mouseUp:(NSEvent*)event {
  [[statusItem_ button] highlight:NO];
  [self updateLayerVisibility];

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
    NSMenu* tempMenu = [menuController menu];

    // Register for temporary menu notifications.
    [self registerMenuNotifications:tempMenu];

    [statusItem_ setMenu:tempMenu];
    base::WeakPtr<electron::TrayIconCocoa> weak_tray_icon =
        trayIcon_->GetWeakPtr();
    [[statusItem_ button] performClick:self];
    // /⚠️ \ Warning! Arbitrary JavaScript and who knows what else has been run
    // during -performClick:. This object may have been deleted.
    // We check if |trayIcon_| is still alive as it owns us and has the same
    // lifetime.
    if (!weak_tray_icon)
      return;

    // Clean up temporary menu notifications and restore original menu.
    [self unregisterMenuNotifications:tempMenu];

    NSMenu* originalMenu = [menuController_ menu];
    [self registerMenuNotifications:originalMenu];
    currentMenu_ = originalMenu;

    [statusItem_ setMenu:originalMenu];
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

namespace {

// Helper to convert std::vector<gfx::Image> to NSMutableArray<NSImage*>.
// Filters out any images that fail to convert to NSImage.
NSMutableArray<NSImage*>* ConvertImagesToNSArray(
    const std::vector<gfx::Image>& layers) {
  NSMutableArray<NSImage*>* nsLayers = [NSMutableArray array];
  for (const auto& img : layers) {
    NSImage* nsImg = img.AsNSImage();
    if (nsImg) {
      [nsLayers addObject:nsImg];
    }
  }
  return nsLayers;
}

}  // namespace

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

void TrayIconCocoa::SetAlternateLayeredImages(
    const std::vector<gfx::Image>& layers) {
  [status_item_view_ setAlternateLayeredImages:ConvertImagesToNSArray(layers)];
}

void TrayIconCocoa::SetLayeredImages(const std::vector<gfx::Image>& layers) {
  [status_item_view_ setLayeredImages:ConvertImagesToNSArray(layers)];
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
