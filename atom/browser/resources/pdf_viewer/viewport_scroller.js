// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * @private
 * The period of time in milliseconds to wait between updating the viewport
 * position by the scroll velocity.
 */
ViewportScroller.DRAG_TIMER_INTERVAL_MS_ = 100;

/**
 * @private
 * The maximum drag scroll distance per DRAG_TIMER_INTERVAL in pixels.
 */
ViewportScroller.MAX_DRAG_SCROLL_DISTANCE_ = 100;

/**
 * Creates a new ViewportScroller.
 * A ViewportScroller scrolls the page in response to drag selection with the
 * mouse.
 * @param {Object} viewport The viewport info of the page.
 * @param {Object} plugin The PDF plugin element.
 * @param {Object} window The window containing the viewer.
 */
function ViewportScroller(viewport, plugin, window) {
  this.viewport_ = viewport;
  this.plugin_ = plugin;
  this.window_ = window;
  this.mousemoveCallback_ = null;
  this.timerId_ = null;
  this.scrollVelocity_ = null;
  this.lastFrameTime_ = 0;
}

ViewportScroller.prototype = {
  /**
   * @private
   * Start scrolling the page by |scrollVelocity_| every
   * |DRAG_TIMER_INTERVAL_MS_|.
   */
  startDragScrollTimer_: function() {
    if (this.timerId_ === null) {
      this.timerId_ =
          this.window_.setInterval(this.dragScrollPage_.bind(this),
                                   ViewportScroller.DRAG_TIMER_INTERVAL_MS_);
      this.lastFrameTime_ = Date.now();
    }
  },

  /**
   * @private
   * Stops the drag scroll timer if it is active.
   */
  stopDragScrollTimer_: function() {
    if (this.timerId_ !== null) {
      this.window_.clearInterval(this.timerId_);
      this.timerId_ = null;
      this.lastFrameTime_ = 0;
    }
  },

  /**
   * @private
   * Scrolls the viewport by the current scroll velocity.
   */
  dragScrollPage_: function() {
    var position = this.viewport_.position;
    var currentFrameTime = Date.now();
    var timeAdjustment = (currentFrameTime - this.lastFrameTime_) /
                         ViewportScroller.DRAG_TIMER_INTERVAL_MS_;
    position.y += (this.scrollVelocity_.y * timeAdjustment);
    position.x += (this.scrollVelocity_.x * timeAdjustment);
    this.viewport_.position = position;
    this.lastFrameTime_ = currentFrameTime;
  },

  /**
   * @private
   * Calculate the velocity to scroll while dragging using the distance of the
   * cursor outside the viewport.
   * @param {Object} event The mousemove event.
   * @return {Object} Object with x and y direction scroll velocity.
   */
  calculateVelocity_: function(event) {
    var x = Math.min(Math.max(-event.offsetX,
                              event.offsetX - this.plugin_.offsetWidth, 0),
                     ViewportScroller.MAX_DRAG_SCROLL_DISTANCE_) *
            Math.sign(event.offsetX);
    var y = Math.min(Math.max(-event.offsetY,
                              event.offsetY - this.plugin_.offsetHeight, 0),
                     ViewportScroller.MAX_DRAG_SCROLL_DISTANCE_) *
            Math.sign(event.offsetY);
    return {
      x: x,
      y: y
    };
  },

  /**
   * @private
   * Handles mousemove events. It updates the scroll velocity and starts and
   * stops timer based on scroll velocity.
   * @param {Object} event The mousemove event.
   */
  onMousemove_: function(event) {
    this.scrollVelocity_ = this.calculateVelocity_(event);
    if (!this.scrollVelocity_.x && !this.scrollVelocity_.y)
      this.stopDragScrollTimer_();
    else if (!this.timerId_)
      this.startDragScrollTimer_();
  },

  /**
   * Sets whether to scroll the viewport when the mouse is outside the
   * viewport.
   * @param {boolean} isSelecting Represents selection status.
   */
  setEnableScrolling: function(isSelecting) {
    if (isSelecting) {
      if (!this.mousemoveCallback_)
        this.mousemoveCallback_ = this.onMousemove_.bind(this);
      this.plugin_.addEventListener('mousemove', this.mousemoveCallback_,
                                    false);
    } else {
      this.stopDragScrollTimer_();
      if (this.mousemoveCallback_) {
        this.plugin_.removeEventListener('mousemove', this.mousemoveCallback_,
                                         false);
      }
    }
  }
};
