// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * A class that manages updating the browser with zoom changes.
 */
class ZoomManager {
  /**
   * Constructs a ZoomManager
   * @param {!Viewport} viewport A Viewport for which to manage zoom.
   * @param {Function} setBrowserZoomFunction A function that sets the browser
   *     zoom to the provided value.
   * @param {number} initialZoom The initial browser zoom level.
   */
  constructor(viewport, setBrowserZoomFunction, initialZoom) {
    this.viewport_ = viewport;
    this.setBrowserZoomFunction_ = setBrowserZoomFunction;
    this.browserZoom_ = initialZoom;
    this.changingBrowserZoom_ = null;
  }

  /**
   * Invoked when a browser-initiated zoom-level change occurs.
   * @param {number} newZoom the zoom level to zoom to.
   */
  onBrowserZoomChange(newZoom) {
    // If we are changing the browser zoom level, ignore any browser zoom level
    // change events. Either, the change occurred before our update and will be
    // overwritten, or the change being reported is the change we are making,
    // which we have already handled.
    if (this.changingBrowserZoom_)
      return;

    if (this.floatingPointEquals(this.browserZoom_, newZoom))
      return;

    this.browserZoom_ = newZoom;
    this.viewport_.setZoom(newZoom);
  }

  /**
   * Invoked when an extension-initiated zoom-level change occurs.
   */
  onPdfZoomChange() {
    // If we are already changing the browser zoom level in response to a
    // previous extension-initiated zoom-level change, ignore this zoom change.
    // Once the browser zoom level is changed, we check whether the extension's
    // zoom level matches the most recently sent zoom level.
    if (this.changingBrowserZoom_)
      return;

    let zoom = this.viewport_.zoom;
    if (this.floatingPointEquals(this.browserZoom_, zoom))
      return;

    this.changingBrowserZoom_ = this.setBrowserZoomFunction_(zoom).then(
        function() {
      this.browserZoom_ = zoom;
      this.changingBrowserZoom_ = null;

      // The extension's zoom level may have changed while the browser zoom
      // change was in progress. We call back into onPdfZoomChange to ensure the
      // browser zoom is up to date.
      this.onPdfZoomChange();
    }.bind(this));
  }

  /**
   * Returns whether two numbers are approximately equal.
   * @param {number} a The first number.
   * @param {number} b The second number.
   */
  floatingPointEquals(a, b) {
    let MIN_ZOOM_DELTA = 0.01;
    // If the zoom level is close enough to the current zoom level, don't
    // change it. This avoids us getting into an infinite loop of zoom changes
    // due to floating point error.
    return Math.abs(a - b) <= MIN_ZOOM_DELTA;
  }
};
