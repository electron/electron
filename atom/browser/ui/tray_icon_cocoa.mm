// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_cocoa.h"

#include "atom/browser/ui/cocoa/atom_menu_controller.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/screen.h"


const CGFloat kStatusItemLength = 26;
const CGFloat kMargin = 3;

@interface StatusItemView : NSView {
  atom::TrayIconCocoa* trayIcon_; // weak
  AtomMenuController* menu_controller_; // weak
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
  NSRect frame = NSMakeRect(0, 0, 26, [[statusItem_ statusBar] thickness]);
  if ((self = [super initWithFrame:frame])) {
    [statusItem_ setView:self];
  }
  return self;
}

- (void)removeItem {
  [[NSStatusBar systemStatusBar] removeStatusItem:statusItem_];
  statusItem_.reset();
}

- (void)drawRect:(NSRect)dirtyRect {
  BOOL highlight = [self shouldHighlight];
  [statusItem_ drawStatusBarBackgroundInRect:[self bounds]
                               withHighlight:highlight];
  NSRect icon_frame = NSMakeRect(0,
                                 0,
                                 kStatusItemLength,
                                 [[statusItem_ statusBar] thickness]);
  NSRect icon_draw_rect = NSInsetRect(icon_frame, kMargin, kMargin);
  if (highlight && alternateImage_) {
    [alternateImage_ drawInRect:icon_draw_rect
                       fromRect:NSZeroRect
                      operation:NSCompositeSourceOver
                       fraction:1];
  } else {
    [image_ drawInRect:icon_draw_rect
              fromRect:NSZeroRect
             operation:NSCompositeSourceOver
              fraction:1];
  }
  if (title_) {
    NSAttributedString* attributes =
        [[NSAttributedString alloc] initWithString:title_
                                        attributes:[self titleAttributes]];
    CGFloat title_width  = [attributes size].width;
    NSRect title_rect = NSMakeRect(kStatusItemLength,
                                   0,
                                   title_width + kStatusItemLength,
                                   [[statusItem_ statusBar] thickness]);
    [title_ drawInRect:title_rect
        withAttributes:[self titleAttributes]];
    [statusItem_ setLength:title_width + kStatusItemLength];
  }
}

- (NSDictionary *)titleAttributes {
  NSFont* font = [NSFont menuBarFontOfSize:0];
  NSColor* foregroundColor = [NSColor blackColor];

  return [NSDictionary dictionaryWithObjectsAndKeys:
          font, NSFontAttributeName,
          foregroundColor, NSForegroundColorAttributeName,
          nil];
}

- (void)setImage:(NSImage*)image {
  image_.reset([image copy]);
}

- (void)setAlternateImage:(NSImage*)image {
  alternateImage_.reset([image copy]);
}

- (void)setHighlight:(BOOL)highlight {
  isHighlightEnable_ = highlight;
}

-(void)setTitle:(NSString*) title {
  //title_= [title retain];
  title_.reset([title copy]);
}

- (void)setMenuController:(AtomMenuController*)menu {
  menu_controller_ = menu;
}

-(void)mouseDown:(NSEvent *)event {
  inMouseEventSequence_ = YES;
  [self setNeedsDisplay:YES];
}

-(void)mouseUp:(NSEvent *)event {
  if (!inMouseEventSequence_) {
     // If the menu is showing, when user clicked the tray icon, the `mouseDown`
     // event will be dissmissed, we need to close the menu at this time.
     [self setNeedsDisplay:YES];
     return;
  }
  inMouseEventSequence_ = NO;
  if (event.clickCount == 1) {
    if (menu_controller_) {
      [statusItem_ popUpStatusItemMenu:[menu_controller_ menu]];
    }

    trayIcon_->NotifyClicked([self getBoundsFromEvent:event]);
  }

  if (event.clickCount == 2 && !menu_controller_) {
    trayIcon_->NotifyDoubleClicked();
  }
  [self setNeedsDisplay:YES];
}

- (void)rightMouseUp:(NSEvent*)event {
  trayIcon_->NotifyRightClicked([self getBoundsFromEvent:event]);
}

-(BOOL) shouldHighlight {
  BOOL is_menu_open = [menu_controller_ isMenuOpen];
  return isHighlightEnable_ && (inMouseEventSequence_ || is_menu_open);
}

-(gfx::Rect) getBoundsFromEvent:(NSEvent*)event {
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

void TrayIconCocoa::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_.reset([[AtomMenuController alloc] initWithModel:menu_model]);
  [status_item_view_ setMenuController:menu_.get()];
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconCocoa;
}

}  // namespace atom
