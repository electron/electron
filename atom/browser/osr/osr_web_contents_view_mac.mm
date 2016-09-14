// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_web_contents_view.h"

#import <Cocoa/Cocoa.h>

@interface OffScreenView : NSView
@end

@implementation OffScreenView

- (void)drawRect:(NSRect)dirtyRect {
  NSString* str = @"No content under offscreen mode";
  NSMutableParagraphStyle* paragraphStyle =
      [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
  [paragraphStyle setAlignment:NSCenterTextAlignment];
  NSDictionary* attributes = [NSDictionary
      dictionaryWithObject:paragraphStyle
                    forKey:NSParagraphStyleAttributeName];
  NSAttributedString* text =
      [[[NSAttributedString alloc] initWithString:str
                                       attributes:attributes] autorelease];
  NSRect frame = NSMakeRect(0, (self.frame.size.height - text.size.height) / 2,
                            self.frame.size.width, text.size.height);
  [str drawInRect:frame withAttributes:attributes];
}

@end

namespace atom {

gfx::NativeView OffScreenWebContentsView::GetNativeView() const {
  return offScreenView_;
}

gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const {
  return offScreenView_;
}

gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const {
  return [offScreenView_ window];
}

void OffScreenWebContentsView::PlatformCreate() {
  offScreenView_ = [[OffScreenView alloc] init];
}

void OffScreenWebContentsView::PlatformDestroy() {
  [offScreenView_ release];
}

}  // namespace atom
