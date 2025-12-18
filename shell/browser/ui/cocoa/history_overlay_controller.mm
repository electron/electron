// Copyright (c) 2024 Electron.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/ui/cocoa/history_overlay_controller.h"

#import <QuartzCore/QuartzCore.h>

#include <algorithm>
#include <cmath>

#include "base/apple/scoped_cftyperef.h"

// Constants ///////////////////////////////////////////////////////////////////

// The radius of the circle drawn in the shield.
const CGFloat kShieldRadius = 70;

// The diameter of the circle and the width of its bounding box.
const CGFloat kShieldWidth = kShieldRadius * 2;

// The height of the shield.
const CGFloat kShieldHeight = 140;

// Additional height that is added to kShieldHeight when the gesture is
// considered complete.
const CGFloat kShieldHeightCompletionAdjust = 10;

// HistoryOverlayView //////////////////////////////////////////////////////////

// The content view that draws the semicircle and the arrow.
@interface HistoryOverlayView : NSView
@property(nonatomic) CGFloat shieldAlpha;
- (instancetype)initWithMode:(electron::HistoryOverlayMode)mode;
@end

@implementation HistoryOverlayView {
  electron::HistoryOverlayMode _mode;
  CAShapeLayer* __strong _shapeLayer;
}

@synthesize shieldAlpha = _shieldAlpha;

- (instancetype)initWithMode:(electron::HistoryOverlayMode)mode {
  NSRect frame = NSMakeRect(0, 0, kShieldWidth, kShieldHeight);
  if ((self = [super initWithFrame:frame])) {
    _mode = mode;
    _shieldAlpha = 1.0;  // CAShapeLayer's fillColor defaults to opaque black.

    // A layer-hosting view.
    _shapeLayer = [[CAShapeLayer alloc] init];
    self.layer = _shapeLayer;
    self.wantsLayer = YES;

    // If going backward, the arrow needs to be in the right half of the circle,
    // so offset the X position.
    CGFloat offset =
        _mode == electron::kHistoryOverlayModeBack ? kShieldRadius : 0;
    NSRect arrowRect = NSMakeRect(offset, 0, kShieldRadius, kShieldHeight);
    arrowRect = NSInsetRect(arrowRect, 10, 0);  // Give a little padding.

    NSImageView* imageView = [[NSImageView alloc] initWithFrame:arrowRect];
    // TODO(electron): Load correct resource images (IDR_SWIPE_BACK/FORWARD)
    // For now we will use a system symbol or simple string if image fails,
    // but ideally we need to bundle the assets.
    // Using a system symbol as placeholder if available (macOS 11+) or drawing
    // simplistic arrow.
    if (@available(macOS 11.0, *)) {
      NSString* symbolName = _mode == electron::kHistoryOverlayModeBack
                                 ? @"arrow.left"
                                 : @"arrow.right";
      imageView.image = [NSImage imageWithSystemSymbolName:symbolName
                                  accessibilityDescription:nil];
      imageView.symbolConfiguration = [NSImageSymbolConfiguration
          configurationWithPointSize:30
                              weight:NSFontWeightBold];
      imageView.contentTintColor = [NSColor whiteColor];
    }

    imageView.autoresizingMask = NSViewMinYMargin | NSViewMaxYMargin;
    [self addSubview:imageView];
  }
  return self;
}

- (void)setFrameSize:(CGSize)newSize {
  NSSize oldSize = self.frame.size;
  [super setFrameSize:newSize];

  if (!_shapeLayer.path || !NSEqualSizes(oldSize, newSize)) {
    base::apple::ScopedCFTypeRef<CGMutablePathRef> oval(CGPathCreateMutable());
    CGRect ovalRect = CGRectMake(0, 0, newSize.width, newSize.height);
    CGPathAddEllipseInRect(oval.get(), nullptr, ovalRect);
    _shapeLayer.path = oval.get();
  }
}

- (void)setShieldAlpha:(CGFloat)shieldAlpha {
  if (shieldAlpha != _shieldAlpha) {
    _shieldAlpha = shieldAlpha;
    base::apple::ScopedCFTypeRef<CGColorRef> fillColor(
        CGColorCreateGenericGray(0, shieldAlpha));
    _shapeLayer.fillColor = fillColor.get();
  }
}

@end

// HistoryOverlayController ////////////////////////////////////////////////////

@implementation HistoryOverlayController {
  electron::HistoryOverlayMode _mode;
  HistoryOverlayView* __strong _contentView;
}

- (instancetype)initForMode:(electron::HistoryOverlayMode)mode {
  if ((self = [super init])) {
    _mode = mode;
  }
  return self;
}

- (void)loadView {
  _contentView = [[HistoryOverlayView alloc] initWithMode:_mode];
  self.view = _contentView;
}

- (void)setProgress:(CGFloat)gestureAmount finished:(BOOL)finished {
  // DCHECK(self.view.superview);
  if (!self.view.superview)
    return;

  NSRect parentFrame = self.view.superview.frame;
  // When tracking the gesture, the height is constant and the alpha value
  // changes from [0.25, 0.65].
  CGFloat height = kShieldHeight;
  CGFloat shieldAlpha =
      std::min(static_cast<CGFloat>(0.65),
               std::max(gestureAmount, static_cast<CGFloat>(0.25)));

  // When the gesture is very likely to be completed (90% in this case), grow
  // the semicircle's height and lock the alpha to 0.75.
  if (finished) {
    height += kShieldHeightCompletionAdjust;
    shieldAlpha = 0.75;
  }

  // Compute the new position based on the progress.
  NSRect frame = self.view.frame;
  frame.size.height = height;
  frame.origin.y = (NSHeight(parentFrame) / 2) - (height / 2);

  CGFloat width = std::min(kShieldRadius * gestureAmount, kShieldRadius);
  if (_mode == electron::kHistoryOverlayModeForward) {
    frame.origin.x = NSMaxX(parentFrame) - width;
  } else if (_mode == electron::kHistoryOverlayModeBack) {
    frame.origin.x = NSMinX(parentFrame) - kShieldWidth + width;
  }

  self.view.frame = frame;
  _contentView.shieldAlpha = shieldAlpha;
}

- (void)showPanelForView:(NSView*)view {
  // The self.view should be added to the hierarchy before its initial position
  // can be set, because setProgress:finished: depends on the superview's frame.
  [view addSubview:self.view];
  [self setProgress:0 finished:NO];  // Set initial view position.
}

- (void)dismiss {
  const CGFloat kFadeOutDurationSeconds = 0.4;

  [NSAnimationContext beginGrouping];
  NSAnimationContext.currentContext.duration = kFadeOutDurationSeconds;
  [self.view.animator removeFromSuperview];
  [NSAnimationContext endGrouping];
}

@end
