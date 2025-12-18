// Copyright (c) 2024 Electron.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/ui/cocoa/history_swiper.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <cmath>
#include <optional>

#import "shell/browser/ui/cocoa/history_overlay_controller.h"
#include "third_party/blink/public/common/input/web_gesture_event.h"
#include "ui/events/blink/did_overscroll_params.h"

namespace {

// The horizontal distance required to cause the browser to perform a history
// navigation.
constexpr CGFloat kHistorySwipeThreshold = 0.08;

// The horizontal distance required for this class to start consuming events,
// which stops the events from reaching the renderer.
constexpr CGFloat kConsumeEventThreshold = 0.01;

// If there has been sufficient vertical motion, the gesture can't be intended
// for history swiping.
constexpr CGFloat kCancelEventVerticalThreshold = 0.24;

// If there has been sufficient vertical motion, and more vertical than
// horizontal motion, the gesture can't be intended for history swiping.
constexpr CGFloat kCancelEventVerticalLowerThreshold = 0.01;

}  // namespace

@interface HistorySwiper ()

// Given a touch event, returns the average touch position.
- (NSPoint)averagePositionInTouchEvent:(NSEvent*)event;

// Updates the internal state with the location information from the given touch
// event.
- (void)updateCurrentPointFromTouchEvent:(NSEvent*)event;

// Updates the state machine with the given touch event. Returns NO if no
// further processing of the event should happen.
- (BOOL)processTouchEventForHistorySwiping:(NSEvent*)event;

// Returns whether the wheel event should be consumed, and not passed to the
// renderer.
- (BOOL)shouldConsumeWheelEvent:(NSEvent*)event;

// Shows the history swiper overlay.
- (void)showHistoryOverlay:
    (electron::history_swiper::NavigationDirection)direction;

// Removes the history swiper overlay.
- (void)removeHistoryOverlay;

// Called to process a scroll wheel event.
- (BOOL)handleScrollWheelEvent:(NSEvent*)event;

// Attempts to initiate history swiping for Magic Mouse events.
- (BOOL)handleMagicMouseWheelEvent:(NSEvent*)theEvent;

@end

@implementation HistorySwiper {
  // This controller will exist if and only if the UI is in history swipe mode.
  HistoryOverlayController* __strong _historyOverlay;

  // Tracks `-touches*WithEvent:` messages.
  BOOL _receivingTouchEvents;

  // --- Touch processing ---

  // The location of the fingers when the touches started.
  NSPoint _touchStartPoint;

  // The current location of the fingers in the touches.
  NSPoint _touchCurrentPoint;

  // The total Y distance moved since the beginning of the touches.
  CGFloat _gestureTotalY;

  // The user's intended direction with the history swipe.
  electron::history_swiper::NavigationDirection _historySwipeDirection;

  // Whether the history swipe gesture has its direction inverted.
  BOOL _historySwipeDirectionInverted;

  // Whether the renderer has consumed the first scroll event.
  BOOL _firstScrollUnconsumed;

  // Whether the overscroll has been triggered by renderer.
  BOOL _overscrollTriggeredByRenderer;

  // Whether we have received a gesture scroll begin and are awaiting the first
  // gesture scroll update.
  BOOL _waitingForFirstGestureScroll;

  // What state the gesture recognition is in.
  electron::history_swiper::RecognitionState _recognitionState;

  // --- Magic Mouse processing ---

  // Cumulative scroll delta since scroll gesture start.
  NSSize _mouseScrollDelta;
}

@synthesize delegate = _delegate;

- (instancetype)initWithDelegate:(id<HistorySwiperDelegate>)delegate {
  self = [super init];
  if (self) {
    _delegate = delegate;
  }
  return self;
}

- (void)dealloc {
  [self removeHistoryOverlay];
}

- (BOOL)handleEvent:(NSEvent*)event {
  if (event.type == NSEventTypeScrollWheel) {
    return [self handleScrollWheelEvent:event];
  }

  return NO;
}

- (void)rendererHandledGestureScrollEvent:(const blink::WebGestureEvent&)event
                                 consumed:(BOOL)consumed {
  switch (event.GetType()) {
    case blink::WebInputEvent::Type::kGestureScrollBegin:
      if (event.data.scroll_begin.synthetic ||
          event.data.scroll_begin.inertial_phase ==
              blink::WebGestureEvent::InertialPhaseState::kMomentum) {
        return;
      }
      _waitingForFirstGestureScroll = YES;
      break;
    case blink::WebInputEvent::Type::kGestureScrollUpdate:
      if (_waitingForFirstGestureScroll) {
        _firstScrollUnconsumed = !consumed;
      }
      _waitingForFirstGestureScroll = NO;
      break;
    default:
      break;
  }
}

- (void)onOverscrolled:(const ui::DidOverscrollParams&)params {
  // We don't have direct access to cc::OverscrollBehavior::Type::kAuto from
  // here easily without adding many deps. However, Electron is built with
  // Chromium so we might have it. Let's assume params structure is same as
  // Chrome. Using 0 (auto) as magic number if needed, but assuming kAuto is
  // default. Actually, let's keep it simple: if there is overscroll, we allow
  // it.
  _overscrollTriggeredByRenderer = YES;
  // Ideally: params.overscroll_behavior.x ==
  // cc::OverscrollBehavior::Type::kAuto;
}

- (NSPoint)averagePositionInTouchEvent:(NSEvent*)event {
  NSPoint position = NSMakePoint(0, 0);
  int pointCount = 0;
  for (NSTouch* touch in [event touchesMatchingPhase:NSTouchPhaseAny
                                              inView:nil]) {
    position.x += touch.normalizedPosition.x;
    position.y += touch.normalizedPosition.y;
    ++pointCount;
  }

  if (pointCount > 1) {
    position.x /= pointCount;
    position.y /= pointCount;
  }

  return position;
}

- (void)updateCurrentPointFromTouchEvent:(NSEvent*)event {
  NSPoint averagePosition = [self averagePositionInTouchEvent:event];
  _gestureTotalY += std::abs(averagePosition.y - _touchCurrentPoint.y);
  _touchCurrentPoint = averagePosition;
}

- (void)touchesBeganWithEvent:(NSEvent*)event {
  _receivingTouchEvents = YES;

  // Reset state pertaining to previous trackpad gestures.
  _touchStartPoint = [self averagePositionInTouchEvent:event];
  _touchCurrentPoint = _touchStartPoint;
  _gestureTotalY = 0.0;
  _firstScrollUnconsumed = NO;
  _overscrollTriggeredByRenderer = NO;
  _waitingForFirstGestureScroll = NO;
}

- (void)touchesMovedWithEvent:(NSEvent*)event {
  [self processTouchEventForHistorySwiping:event];
}

- (void)touchesCancelledWithEvent:(NSEvent*)event {
  _receivingTouchEvents = NO;
  if (![self processTouchEventForHistorySwiping:event]) {
    return;
  }

  [self cancelHistorySwipe];
}

- (void)touchesEndedWithEvent:(NSEvent*)event {
  _receivingTouchEvents = NO;
  if (![self processTouchEventForHistorySwiping:event]) {
    return;
  }

  if (_historyOverlay) {
    BOOL finished = [self updateProgressBar];

    // If the gesture was completed, perform a navigation.
    if (finished) {
      [self navigateBrowserInDirection:_historySwipeDirection];
    }

    [self removeHistoryOverlay];

    // The gesture was completed.
    _recognitionState = electron::history_swiper::kCompleted;
  }
}

- (BOOL)processTouchEventForHistorySwiping:(NSEvent*)event {
  switch (_recognitionState) {
    case electron::history_swiper::kCancelled:
    case electron::history_swiper::kCompleted:
      return NO;
    case electron::history_swiper::kPending:
    case electron::history_swiper::kPotential:
    case electron::history_swiper::kTracking:
      break;
  }

  [self updateCurrentPointFromTouchEvent:event];

  // Consider cancelling the history swipe gesture.
  if ([self shouldCancelHorizontalSwipeWithCurrentPoint:_touchCurrentPoint
                                             startPoint:_touchStartPoint]) {
    [self cancelHistorySwipe];
    return NO;
  }

  // Don't do any more processing if the state machine is in the pending state.
  if (_recognitionState == electron::history_swiper::kPending) {
    return NO;
  }

  if (_recognitionState == electron::history_swiper::kPotential) {
    // The user is in the process of doing history swiping.
    BOOL sufficientlyFar = std::abs(_touchCurrentPoint.x - _touchStartPoint.x) >
                           kConsumeEventThreshold;
    if (sufficientlyFar) {
      _recognitionState = electron::history_swiper::kTracking;

      if (_historySwipeDirection == electron::history_swiper::kBackwards) {
        [_delegate backwardsSwipeNavigationLikely];
      }
    }
  }

  if (_historyOverlay) {
    [self updateProgressBar];
  }
  return YES;
}

// Consider cancelling the horizontal swipe if the user was intending a
// vertical swipe.
- (BOOL)shouldCancelHorizontalSwipeWithCurrentPoint:(NSPoint)currentPoint
                                         startPoint:(NSPoint)startPoint {
  CGFloat yDelta = _gestureTotalY;
  CGFloat xDelta = std::abs(currentPoint.x - startPoint.x);

  // The gesture is pretty clearly more vertical than horizontal.
  if (yDelta > 2 * xDelta) {
    return YES;
  }

  // There's been more vertical distance than horizontal distance.
  if (yDelta * 1.3 > xDelta && yDelta > kCancelEventVerticalLowerThreshold) {
    return YES;
  }

  // There's been a lot of vertical distance.
  if (yDelta > kCancelEventVerticalThreshold) {
    return YES;
  }

  return NO;
}

- (void)cancelHistorySwipe {
  [self removeHistoryOverlay];
  _recognitionState = electron::history_swiper::kCancelled;
}

- (void)removeHistoryOverlay {
  [_historyOverlay dismiss];
  _historyOverlay = nil;
}

// Returns whether the progress bar has been 100% filled.
- (BOOL)updateProgressBar {
  NSPoint currentPoint = _touchCurrentPoint;
  NSPoint startPoint = _touchStartPoint;

  CGFloat progress = 0;
  BOOL finished = NO;

  progress = (currentPoint.x - startPoint.x) / kHistorySwipeThreshold;
  // If the swipe is a backwards gesture, we need to invert progress.
  if (_historySwipeDirection == electron::history_swiper::kBackwards) {
    progress *= -1;
  }

  // If the user has directions reversed, we need to invert progress.
  if (_historySwipeDirectionInverted) {
    progress *= -1;
  }

  if (progress >= 1.0) {
    finished = YES;
  }

  // Progress can't be less than 0 or greater than 1.
  progress = std::clamp(progress, 0.0, 1.0);

  [_historyOverlay setProgress:progress finished:finished];

  return finished;
}

- (void)showHistoryOverlay:
    (electron::history_swiper::NavigationDirection)direction {
  [self removeHistoryOverlay];

  HistoryOverlayController* historyOverlay = [[HistoryOverlayController alloc]
      initForMode:(direction == electron::history_swiper::kForwards)
                      ? electron::kHistoryOverlayModeForward
                      : electron::kHistoryOverlayModeBack];
  [historyOverlay showPanelForView:[_delegate viewThatWantsHistoryOverlay]];
  _historyOverlay = historyOverlay;
}

- (void)navigateBrowserInDirection:
    (electron::history_swiper::NavigationDirection)direction {
  [_delegate navigateInDirection:direction
                        onWindow:_historyOverlay.view.window];
}

- (BOOL)browserCanNavigateInDirection:
            (electron::history_swiper::NavigationDirection)direction
                                event:(NSEvent*)event {
  return [_delegate canNavigateInDirection:direction onWindow:[event window]];
}

- (BOOL)handleMagicMouseWheelEvent:(NSEvent*)theEvent {
  // The `-trackSwipeEventWithOptions:` API doesn't handle momentum events.
  if (theEvent.phase == NSEventPhaseNone) {
    return NO;
  }

  _mouseScrollDelta.width += theEvent.scrollingDeltaX;
  _mouseScrollDelta.height += theEvent.scrollingDeltaY;

  BOOL isHorizontalGesture =
      std::abs(_mouseScrollDelta.width) > std::abs(_mouseScrollDelta.height);

  if (!isHorizontalGesture) {
    return NO;
  }

  BOOL isRightScroll = theEvent.scrollingDeltaX < 0;
  electron::history_swiper::NavigationDirection direction =
      isRightScroll ? electron::history_swiper::kForwards
                    : electron::history_swiper::kBackwards;
  BOOL browserCanMove = [self browserCanNavigateInDirection:direction
                                                      event:theEvent];
  if (!browserCanMove) {
    return NO;
  }

  [self initiateMagicMouseHistorySwipe:isRightScroll event:theEvent];
  return YES;
}

- (void)initiateMagicMouseHistorySwipe:(BOOL)isRightScroll
                                 event:(NSEvent*)event {
  __block HistoryOverlayController* historyOverlay =
      [[HistoryOverlayController alloc]
          initForMode:isRightScroll ? electron::kHistoryOverlayModeForward
                                    : electron::kHistoryOverlayModeBack];

  [event trackSwipeEventWithOptions:NSEventSwipeTrackingLockDirection
           dampenAmountThresholdMin:-1
                                max:1
                       usingHandler:^(CGFloat gestureAmount, NSEventPhase phase,
                                      BOOL isComplete, BOOL* stop) {
                         if (phase == NSEventPhaseBegan) {
                           [historyOverlay
                               showPanelForView:
                                   [self.delegate viewThatWantsHistoryOverlay]];
                           return;
                         }

                         BOOL ended = phase == NSEventPhaseEnded;
                         CGFloat progress = std::abs(gestureAmount) / 0.3;
                         BOOL finished = progress >= 1.0;
                         progress = std::clamp(progress, 0.0, 1.0);
                         [historyOverlay setProgress:progress
                                            finished:finished];

                         if (ended) {
                           [self.delegate
                               navigateInDirection:
                                   isRightScroll
                                       ? electron::history_swiper::kForwards
                                       : electron::history_swiper::kBackwards
                                          onWindow:historyOverlay.view.window];
                         }

                         if (ended || isComplete) {
                           [historyOverlay dismiss];
                           historyOverlay = nil;
                         }
                       }];
}

- (BOOL)handleScrollWheelEvent:(NSEvent*)theEvent {
  if (theEvent.phase == NSEventPhaseBegan) {
    _recognitionState = electron::history_swiper::kPending;
    _mouseScrollDelta = NSZeroSize;
    // For Electron, we default to allowing swipe since we don't have the
    // complex renderer ACK system that Chrome uses.
    _firstScrollUnconsumed = YES;
    _overscrollTriggeredByRenderer = YES;
  }

  if (theEvent.phase != NSEventPhaseChanged &&
      theEvent.momentumPhase != NSEventPhaseChanged) {
    return NO;
  }

  // We've already processed this gesture and are tracking or completed.
  if (_recognitionState != electron::history_swiper::kPending) {
    return [self shouldConsumeWheelEvent:theEvent];
  }

  // Don't allow momentum events to start history swiping.
  if (theEvent.momentumPhase != NSEventPhaseNone) {
    return NO;
  }

  if (!NSEvent.swipeTrackingFromScrollEventsEnabled) {
    return NO;
  }

  if (![_delegate shouldAllowHistorySwiping]) {
    return NO;
  }

  // If we're not receiving touch events (Magic Mouse), use the Magic Mouse
  // handling path.
  if (!_receivingTouchEvents) {
    return [self handleMagicMouseWheelEvent:theEvent];
  }

  // Need at least some horizontal movement to determine direction.
  CGFloat xDelta = _touchCurrentPoint.x - _touchStartPoint.x;
  if (std::abs(xDelta) < 0.001) {
    return NO;
  }

  BOOL isRightScroll = xDelta > 0;
  if (theEvent.directionInvertedFromDevice) {
    isRightScroll = !isRightScroll;
  }

  electron::history_swiper::NavigationDirection direction =
      isRightScroll ? electron::history_swiper::kForwards
                    : electron::history_swiper::kBackwards;
  BOOL browserCanMove = [self browserCanNavigateInDirection:direction
                                                      event:theEvent];
  if (!browserCanMove) {
    return NO;
  }

  _historySwipeDirection = direction;
  _historySwipeDirectionInverted = theEvent.directionInvertedFromDevice;
  _recognitionState = electron::history_swiper::kPotential;
  [self showHistoryOverlay:direction];
  return [self shouldConsumeWheelEvent:theEvent];
}

- (BOOL)shouldConsumeWheelEvent:(NSEvent*)event {
  switch (_recognitionState) {
    case electron::history_swiper::kPending:
    case electron::history_swiper::kCancelled:
      return NO;
    case electron::history_swiper::kTracking:
    case electron::history_swiper::kCompleted:
      return YES;
    case electron::history_swiper::kPotential:
      return event.scrollingDeltaY == 0;
  }
}

@end
