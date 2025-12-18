// Copyright (c) 2024 Electron.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_HISTORY_SWIPER_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_HISTORY_SWIPER_H_

#import <Cocoa/Cocoa.h>

namespace blink {
class WebGestureEvent;
}  // namespace blink

namespace ui {
struct DidOverscrollParams;
}  // namespace ui

namespace electron {

namespace history_swiper {

enum NavigationDirection {
  kBackwards = 0,
  kForwards,
};

// The states of a state machine that recognizes the history swipe gesture.
enum RecognitionState {
  kPending,
  kPotential,
  kTracking,
  kCompleted,
  kCancelled,
};

}  // namespace history_swiper

}  // namespace electron

@protocol HistorySwiperDelegate
- (BOOL)shouldAllowHistorySwiping;
- (NSView*)viewThatWantsHistoryOverlay;
- (BOOL)canNavigateInDirection:
            (electron::history_swiper::NavigationDirection)direction
                      onWindow:(NSWindow*)window;
- (void)navigateInDirection:
            (electron::history_swiper::NavigationDirection)direction
                   onWindow:(NSWindow*)window;
- (void)backwardsSwipeNavigationLikely;
@end

@interface HistorySwiper : NSObject

@property(nonatomic, weak) id<HistorySwiperDelegate> delegate;

- (instancetype)initWithDelegate:(id<HistorySwiperDelegate>)delegate;

- (BOOL)handleEvent:(NSEvent*)event;
- (void)rendererHandledGestureScrollEvent:(const blink::WebGestureEvent&)event
                                 consumed:(BOOL)consumed;
- (void)onOverscrolled:(const ui::DidOverscrollParams&)params;

- (void)touchesBeganWithEvent:(NSEvent*)event;
- (void)touchesMovedWithEvent:(NSEvent*)event;
- (void)touchesCancelledWithEvent:(NSEvent*)event;
- (void)touchesEndedWithEvent:(NSEvent*)event;

@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_HISTORY_SWIPER_H_
