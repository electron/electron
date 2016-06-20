// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_cocoa.h"

#include "atom/browser/ui/cocoa/atom_menu_controller.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/events/cocoa/cocoa_event_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/screen.h"

namespace {

// By default, macOS sets 4px to tray image as left and right padding margin.
const CGFloat kHorizontalMargin = 4;
// macOS tends to make the title 2px lower.
const CGFloat kVerticalTitleMargin = 2;

}  //  namespace

@interface StatusItemView : NSView {
  atom::TrayIconCocoa* trayIcon_; // weak
  AtomMenuController* menuController_; // weak
  BOOL isHighlightEnable_;
  BOOL forceHighlight_;
  BOOL inMouseEventSequence_;
  base::scoped_nsobject<NSImage> image_;
  base::scoped_nsobject<NSImage> alternateImage_;
  base::scoped_nsobject<NSString> title_;
  base::scoped_nsobject<NSStatusItem> statusItem_;
}

@end  // @interface StatusItemView

@implementation StatusItemView

- (id)initWithImage:(NSImage*)image icon:(atom::TrayIconCocoa*)icon {
  image_.reset([image copy]);
  trayIcon_ = icon;
  isHighlightEnable_ = YES;
  forceHighlight_ = NO;
  inMouseEventSequence_ = NO;

  if ((self = [super initWithFrame: CGRectZero])) {
    [self registerForDraggedTypes: @[NSFilenamesPboardType]];

    // Create the status item.
    NSStatusItem * item = [[NSStatusBar systemStatusBar]
                            statusItemWithLength:NSVariableStatusItemLength];
    statusItem_.reset([item retain]);
    [statusItem_ setView:self];

    // Finalize setup by sizing our views
    [self updateDimensions];
  }
  return self;
}

- (void)updateDimensions {
  NSStatusBar * bar = [NSStatusBar systemStatusBar];
  [self setFrame: NSMakeRect(0, 0, [self fullWidth], [bar thickness])];
  [self setNeedsDisplay:YES];
}

- (void)removeItem {
  [[NSStatusBar systemStatusBar] removeStatusItem:statusItem_];
  statusItem_.reset();
}

- (void)drawRect:(NSRect)dirtyRect {
  // Draw the tray icon and title that align with NSStatusItem, layout:
  //   ----------------
  //   | icon | title |
  ///  ----------------

  BOOL highlight = [self shouldHighlight];
  BOOL highlightContent = highlight | [self isDarkMode];
  CGFloat thickness = [[statusItem_ statusBar] thickness];

  // Draw the system bar background.
  [statusItem_ drawStatusBarBackgroundInRect:self.bounds withHighlight:highlight];

  // Determine which image to use.
  NSImage* image = image_.get();
  if (inMouseEventSequence_ && alternateImage_) {
    image = alternateImage_.get();
  }
  // Apply the higlight color if the image is a template image. When this moves
  // to using the new [NSStatusItem button] API, this should work automagically.
  if ([image isTemplate] == YES) {
    NSImage* imageWithColor = [[image copy] autorelease];
    [imageWithColor lockFocus];
    [[self colorWithHighlight: highlightContent] set];
    CGRect imageBounds = CGRectMake(0,0, image.size.width, image.size.height);
    NSRectFillUsingOperation(imageBounds, NSCompositeSourceAtop);
    [imageWithColor unlockFocus];
    image = imageWithColor;
  }

  // Draw the image
  [image drawInRect: CGRectMake(
    roundf(([self iconWidth] - image.size.width) / 2),
    roundf((thickness - image.size.height) / 2),
    image.size.width,
    image.size.height
  )];

  if (title_) {
    // Draw title.
    NSRect titleDrawRect = NSMakeRect(
        [self iconWidth], -kVerticalTitleMargin, [self titleWidth], thickness);
    [title_ drawInRect:titleDrawRect
        withAttributes:[self titleAttributesWithHighlight:highlightContent]];
  }
}

- (BOOL)isDarkMode {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* mode = [defaults stringForKey:@"AppleInterfaceStyle"];
  return mode && [mode isEqualToString:@"Dark"];
}

// The width of the full status item.
- (CGFloat)fullWidth {
  if (title_)
    return [self iconWidth] + [self titleWidth] + kHorizontalMargin;
  else
    return [self iconWidth];
}

// The width of the icon.
- (CGFloat)iconWidth {
  CGFloat thickness = [[NSStatusBar systemStatusBar] thickness];
  CGFloat imageHeight = [image_ size].height;
  CGFloat imageWidth = [image_ size].width;
  CGFloat iconWidth = imageWidth;
  if (imageWidth < thickness) {
    // Image's width must be larger than menu bar's height.
    iconWidth = thickness;
  } else {
    CGFloat verticalMargin = thickness - imageHeight;
    // Image must have same horizontal vertical margin.
    if (verticalMargin > 0 && imageWidth != imageHeight)
      iconWidth = imageWidth + verticalMargin;
    CGFloat horizontalMargin = thickness - imageWidth;
    // Image must have at least kHorizontalMargin horizontal margin on each
    // side.
    if (horizontalMargin < 2 * kHorizontalMargin)
      iconWidth = imageWidth + 2 * kHorizontalMargin;
  }
  return iconWidth;
}

// The width of the title.
- (CGFloat)titleWidth {
  if (!title_)
     return 0;
  base::scoped_nsobject<NSAttributedString> attributes(
      [[NSAttributedString alloc] initWithString:title_
                                      attributes:[self titleAttributes]]);
  return [attributes size].width;
}

- (NSColor*)colorWithHighlight:(BOOL)highlight {
  return highlight ?
      [NSColor whiteColor] :
      [NSColor colorWithRed:0.265625 green:0.25390625 blue:0.234375 alpha:1.0];
}

- (NSDictionary*)titleAttributesWithHighlight:(BOOL)highlight {
  return @{
      NSFontAttributeName:[NSFont menuBarFontOfSize:0],
      NSForegroundColorAttributeName:[self colorWithHighlight: highlight]
  };
}

- (NSDictionary*)titleAttributes {
  return [self titleAttributesWithHighlight:[self isDarkMode]];
}

- (void)setImage:(NSImage*)image {
  image_.reset([image copy]);
  [self updateDimensions];
}

- (void)setAlternateImage:(NSImage*)image {
  alternateImage_.reset([image copy]);
}

- (void)setHighlight:(BOOL)highlight {
  isHighlightEnable_ = highlight;
}

- (void)setTitle:(NSString*)title {
  if (title.length > 0) {
    title_.reset([title copy]);
  } else {
    title_.reset();
  }
  [self updateDimensions];
}

- (void)setMenuController:(AtomMenuController*)menu {
  menuController_ = menu;
}

- (void)mouseDown:(NSEvent*)event {
  inMouseEventSequence_ = YES;
  [self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent*)event {
  if (!inMouseEventSequence_) {
     // If the menu is showing, when user clicked the tray icon, the `mouseDown`
     // event will be dissmissed, we need to close the menu at this time.
     [self setNeedsDisplay:YES];
     return;
  }
  inMouseEventSequence_ = NO;

  // Show menu when there is a context menu.
  // NB(hokein): Make tray's behavior more like official one's.
  // When the tray icon gets clicked quickly multiple times, the
  // event.clickCount doesn't always return 1. Instead, it returns a value that
  // counts the clicked times.
  // So we don't check the clickCount here, just pop up the menu for each click
  // event.
  if (menuController_)
    [statusItem_ popUpStatusItemMenu:[menuController_ menu]];

  // Don't emit click events when menu is showing.
  if (menuController_)
    return;

  // Single click event.
  if (event.clickCount == 1)
    trayIcon_->NotifyClicked(
        [self getBoundsFromEvent:event],
        ui::EventFlagsFromModifiers([event modifierFlags]));

  // Double click event.
  if (event.clickCount == 2)
    trayIcon_->NotifyDoubleClicked(
        [self getBoundsFromEvent:event],
        ui::EventFlagsFromModifiers([event modifierFlags]));

  [self setNeedsDisplay:YES];
}

- (void)popUpContextMenu:(ui::SimpleMenuModel*)menu_model {
  // Show a custom menu.
  if (menu_model) {
    base::scoped_nsobject<AtomMenuController> menuController(
        [[AtomMenuController alloc] initWithModel:menu_model]);
    forceHighlight_ = YES;  // Should highlight when showing menu.
    [self setNeedsDisplay:YES];
    [statusItem_ popUpStatusItemMenu:[menuController menu]];
    forceHighlight_ = NO;
    [self setNeedsDisplay:YES];
    return;
  }

  if (menuController_ && ![menuController_ isMenuOpen]) {
    // Redraw the dray icon to show highlight if it is enabled.
    [self setNeedsDisplay:YES];
    [statusItem_ popUpStatusItemMenu:[menuController_ menu]];
    // The popUpStatusItemMenu returns only after the showing menu is closed.
    // When it returns, we need to redraw the tray icon to not show highlight.
    [self setNeedsDisplay:YES];
  }
}

- (void)rightMouseUp:(NSEvent*)event {
  trayIcon_->NotifyRightClicked(
      [self getBoundsFromEvent:event],
      ui::EventFlagsFromModifiers([event modifierFlags]));
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
  trayIcon_->NotifyDragEntered();
  return NSDragOperationCopy;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender {
  trayIcon_->NotifyDragExited();
}

- (void)draggingEnded:(id <NSDraggingInfo>)sender {
  trayIcon_->NotifyDragEnded();

  if (NSPointInRect([sender draggingLocation], self.frame)) {
    trayIcon_->NotifyDrop();
    [self handleDrop:sender];
  }
}

- (BOOL)handleDrop:(id <NSDraggingInfo>)sender {
  NSPasteboard* pboard = [sender draggingPasteboard];

  if ([[pboard types] containsObject:NSFilenamesPboardType]) {
    std::vector<std::string> dropFiles;
    NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
    for (NSString* file in files)
      dropFiles.push_back(base::SysNSStringToUTF8(file));
    trayIcon_->NotifyDropFiles(dropFiles);
    return YES;
  }
  return NO;
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender {
  return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender {
  return YES;
}

- (BOOL)shouldHighlight {
  if (isHighlightEnable_ && forceHighlight_)
    return true;
  BOOL isMenuOpen = menuController_ && [menuController_ isMenuOpen];
  return isHighlightEnable_ && (inMouseEventSequence_ || isMenuOpen);
}

- (gfx::Rect)getBoundsFromEvent:(NSEvent*)event {
  NSRect frame = event.window.frame;
  gfx::Rect bounds(frame.origin.x, 0, NSWidth(frame), NSHeight(frame));
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];
  bounds.set_y(NSHeight([screen frame]) - NSMaxY(frame));
  return bounds;
}
@end

namespace atom {

TrayIconCocoa::TrayIconCocoa() : menu_model_(nullptr) {
}

TrayIconCocoa::~TrayIconCocoa() {
  [status_item_view_ removeItem];
  if (menu_model_)
    menu_model_->RemoveObserver(this);
}

void TrayIconCocoa::SetImage(const gfx::Image& image) {
  if (status_item_view_) {
    [status_item_view_ setImage:image.AsNSImage()];
  } else {
    status_item_view_.reset(
        [[StatusItemView alloc] initWithImage:image.AsNSImage()
                                         icon:this]);
  }
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

void TrayIconCocoa::SetHighlightMode(bool highlight) {
  [status_item_view_ setHighlight:highlight];
}

void TrayIconCocoa::PopUpContextMenu(const gfx::Point& pos,
                                     ui::SimpleMenuModel* menu_model) {
  [status_item_view_ popUpContextMenu:menu_model];
}

void TrayIconCocoa::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  // Substribe to MenuClosed event.
  if (menu_model_)
    menu_model_->RemoveObserver(this);
  static_cast<AtomMenuModel*>(menu_model)->AddObserver(this);

  // Create native menu.
  menu_.reset([[AtomMenuController alloc] initWithModel:menu_model]);
  [status_item_view_ setMenuController:menu_.get()];
}

void TrayIconCocoa::MenuClosed() {
  [status_item_view_ setNeedsDisplay:YES];
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconCocoa;
}

}  // namespace atom
