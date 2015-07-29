// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_cocoa.h"

#include "atom/browser/ui/cocoa/atom_menu_controller.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/screen.h"

namespace {

const CGFloat kStatusItemLength = 26;
const CGFloat kMargin = 3;

}  //  namespace

@interface StatusItemView : NSView {
  atom::TrayIconCocoa* trayIcon_; // weak
  AtomMenuController* menuController_; // weak
  BOOL isHighlightEnable_;
  BOOL inMouseEventSequence_;
  base::scoped_nsobject<NSImage> image_;
  base::scoped_nsobject<NSImage> alternateImage_;
  base::scoped_nsobject<NSString> title_;
  base::scoped_nsobject<NSStatusItem> statusItem_;
}

@end  // @interface StatusItemView

@implementation StatusItemView

- (id)initWithIcon:(atom::TrayIconCocoa*)icon {
  trayIcon_ = icon;
  isHighlightEnable_ = YES;
  statusItem_.reset([[[NSStatusBar systemStatusBar] statusItemWithLength:
      NSVariableStatusItemLength] retain]);
  NSRect frame = NSMakeRect(0,
                            0,
                            kStatusItemLength,
                            [[statusItem_ statusBar] thickness]);
  if ((self = [super initWithFrame:frame])) {
    [self registerForDraggedTypes:
        [NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
    [statusItem_ setView:self];
  }
  return self;
}

- (void)removeItem {
  [[NSStatusBar systemStatusBar] removeStatusItem:statusItem_];
  statusItem_.reset();
}

- (void)drawRect:(NSRect)dirtyRect {
  // Draw the tray icon and title that align with NSSStatusItem, layout:
  //   ----------------
  //   | icon | title |
  ///  ----------------
  BOOL highlight = [self shouldHighlight];
  CGFloat titleWidth = [self titleWidth];
  // Calculate the total icon bounds.
  NSRect statusItemBounds = NSMakeRect(0,
                                       0,
                                       kStatusItemLength + titleWidth,
                                       [[statusItem_ statusBar] thickness]);
  [statusItem_ drawStatusBarBackgroundInRect:statusItemBounds
                               withHighlight:highlight];
  [statusItem_ setLength:titleWidth + kStatusItemLength];
  if (title_) {
    NSRect titleDrawRect = NSMakeRect(kStatusItemLength,
                                      0,
                                      titleWidth + kStatusItemLength,
                                      [[statusItem_ statusBar] thickness]);
    [title_ drawInRect:titleDrawRect
        withAttributes:[self titleAttributes]];
  }

  NSRect iconRect = NSMakeRect(0,
                               0,
                               kStatusItemLength,
                               [[statusItem_ statusBar] thickness]);
  if (highlight && alternateImage_) {
    [alternateImage_ drawInRect:NSInsetRect(iconRect, kMargin, kMargin)
                       fromRect:NSZeroRect
                      operation:NSCompositeSourceOver
                       fraction:1];
  } else {
    [image_ drawInRect:NSInsetRect(iconRect, kMargin, kMargin)
              fromRect:NSZeroRect
             operation:NSCompositeSourceOver
              fraction:1];
  }
}

- (CGFloat)titleWidth {
  if (!title_) return 0;
  NSAttributedString* attributes =
      [[NSAttributedString alloc] initWithString:title_
                                      attributes:[self titleAttributes]];
  return [attributes size].width;
}

- (NSDictionary*)titleAttributes {
  NSFont* font = [NSFont menuBarFontOfSize:0];
  NSColor* foregroundColor = [NSColor blackColor];

  return [NSDictionary dictionaryWithObjectsAndKeys:
          font, NSFontAttributeName,
          foregroundColor, NSForegroundColorAttributeName,
          nil];
}

- (void)setImage:(NSImage*)image {
  image_.reset([image copy]);
  [self setNeedsDisplay:YES];
}

- (void)setAlternateImage:(NSImage*)image {
  alternateImage_.reset([image copy]);
}

- (void)setHighlight:(BOOL)highlight {
  isHighlightEnable_ = highlight;
}

- (void)setTitle:(NSString*)title {
  title_.reset([title copy]);
  [self setNeedsDisplay:YES];
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
  if (event.clickCount == 1) {
    if (menuController_) {
      [statusItem_ popUpStatusItemMenu:[menuController_ menu]];
    }

    trayIcon_->NotifyClicked([self getBoundsFromEvent:event]);
  }

  if (event.clickCount == 2 && !menuController_) {
    trayIcon_->NotifyDoubleClicked([self getBoundsFromEvent:event]);
  }
  [self setNeedsDisplay:YES];
}

- (void)popContextMenu {
  if (menuController_ && ![menuController_ isMenuOpen]) {
    // redraw the dray icon to show highlight if it is enabled.
    [self setNeedsDisplay:YES];
    [statusItem_ popUpStatusItemMenu:[menuController_ menu]];
    // The popUpStatusItemMenu returns only after the showing menu is closed.
    // When it returns, we need to redraw the tray icon to not show highlight.
    [self setNeedsDisplay:YES];
  }
}

- (void)rightMouseUp:(NSEvent*)event {
  trayIcon_->NotifyRightClicked([self getBoundsFromEvent:event]);
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
  return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender {
  NSPasteboard* pboard = [sender draggingPasteboard];

  if ([[pboard types] containsObject:NSFilenamesPboardType]) {
    std::vector<std::string> dropFiles;
    NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
    for (NSString* file in files)
      dropFiles.push_back(base::SysNSStringToUTF8(file));
    trayIcon_->NotfiyDropFiles(dropFiles);
    return YES;
  }
  return NO;
}

- (BOOL)shouldHighlight {
  BOOL is_menu_open = [menuController_ isMenuOpen];
  return isHighlightEnable_ && (inMouseEventSequence_ || is_menu_open);
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

TrayIconCocoa::TrayIconCocoa() {
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

void TrayIconCocoa::SetHighlightMode(bool highlight) {
  [status_item_view_ setHighlight:highlight];
}

void TrayIconCocoa::PopContextMenu(const gfx::Point& pos) {
  [status_item_view_ popContextMenu];
}

void TrayIconCocoa::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_.reset([[AtomMenuController alloc] initWithModel:menu_model]);
  [status_item_view_ setMenuController:menu_.get()];
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconCocoa;
}

}  // namespace atom
