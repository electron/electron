// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Returns the height of the intersection of two rectangles.
 * @param {Object} rect1 the first rect
 * @param {Object} rect2 the second rect
 * @return {number} the height of the intersection of the rects
 */
function getIntersectionHeight(rect1, rect2) {
  return Math.max(0,
      Math.min(rect1.y + rect1.height, rect2.y + rect2.height) -
      Math.max(rect1.y, rect2.y));
}

/**
 * Create a new viewport.
 * @constructor
 * @param {Window} window the window
 * @param {Object} sizer is the element which represents the size of the
 *     document in the viewport
 * @param {Function} viewportChangedCallback is run when the viewport changes
 * @param {Function} beforeZoomCallback is run before a change in zoom
 * @param {Function} afterZoomCallback is run after a change in zoom
 * @param {number} scrollbarWidth the width of scrollbars on the page
 * @param {number} defaultZoom The default zoom level.
 * @param {number} topToolbarHeight The number of pixels that should initially
 *     be left blank above the document for the toolbar.
 */
function Viewport(window,
                  sizer,
                  viewportChangedCallback,
                  beforeZoomCallback,
                  afterZoomCallback,
                  scrollbarWidth,
                  defaultZoom,
                  topToolbarHeight) {
  this.window_ = window;
  this.sizer_ = sizer;
  this.viewportChangedCallback_ = viewportChangedCallback;
  this.beforeZoomCallback_ = beforeZoomCallback;
  this.afterZoomCallback_ = afterZoomCallback;
  this.allowedToChangeZoom_ = false;
  this.zoom_ = 1;
  this.documentDimensions_ = null;
  this.pageDimensions_ = [];
  this.scrollbarWidth_ = scrollbarWidth;
  this.fittingType_ = Viewport.FittingType.NONE;
  this.defaultZoom_ = defaultZoom;
  this.topToolbarHeight_ = topToolbarHeight;

  window.addEventListener('scroll', this.updateViewport_.bind(this));
  window.addEventListener('resize', this.resize_.bind(this));
}

/**
 * Enumeration of page fitting types.
 * @enum {string}
 */
Viewport.FittingType = {
  NONE: 'none',
  FIT_TO_PAGE: 'fit-to-page',
  FIT_TO_WIDTH: 'fit-to-width'
};

/**
 * The increment to scroll a page by in pixels when up/down/left/right arrow
 * keys are pressed. Usually we just let the browser handle scrolling on the
 * window when these keys are pressed but in certain cases we need to simulate
 * these events.
 */
Viewport.SCROLL_INCREMENT = 40;

/**
 * Predefined zoom factors to be used when zooming in/out. These are in
 * ascending order. This should match the lists in
 * components/ui/zoom/page_zoom_constants.h and
 * chrome/browser/resources/settings/appearance_page/appearance_page.js
 */
Viewport.ZOOM_FACTORS = [0.25, 1 / 3, 0.5, 2 / 3, 0.75, 0.8, 0.9,
                         1, 1.1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4, 5];

/**
 * The minimum and maximum range to be used to clip zoom factor.
 */
Viewport.ZOOM_FACTOR_RANGE = {
  min: Viewport.ZOOM_FACTORS[0],
  max: Viewport.ZOOM_FACTORS[Viewport.ZOOM_FACTORS.length - 1]
};

/**
 * The width of the page shadow around pages in pixels.
 */
Viewport.PAGE_SHADOW = {top: 3, bottom: 7, left: 5, right: 5};

Viewport.prototype = {
  /**
   * Returns the zoomed and rounded document dimensions for the given zoom.
   * Rounding is necessary when interacting with the renderer which tends to
   * operate in integral values (for example for determining if scrollbars
   * should be shown).
   * @param {number} zoom The zoom to use to compute the scaled dimensions.
   * @return {Object} A dictionary with scaled 'width'/'height' of the document.
   * @private
   */
  getZoomedDocumentDimensions_: function(zoom) {
    if (!this.documentDimensions_)
      return null;
    return {
      width: Math.round(this.documentDimensions_.width * zoom),
      height: Math.round(this.documentDimensions_.height * zoom)
    };
  },

  /**
   * @private
   * Returns true if the document needs scrollbars at the given zoom level.
   * @param {number} zoom compute whether scrollbars are needed at this zoom
   * @return {Object} with 'horizontal' and 'vertical' keys which map to bool
   *     values indicating if the horizontal and vertical scrollbars are needed
   *     respectively.
   */
  documentNeedsScrollbars_: function(zoom) {
    var zoomedDimensions = this.getZoomedDocumentDimensions_(zoom);
    if (!zoomedDimensions) {
      return {
        horizontal: false,
        vertical: false
      };
    }

    // If scrollbars are required for one direction, expand the document in the
    // other direction to take the width of the scrollbars into account when
    // deciding whether the other direction needs scrollbars.
    if (zoomedDimensions.width > this.window_.innerWidth)
      zoomedDimensions.height += this.scrollbarWidth_;
    else if (zoomedDimensions.height > this.window_.innerHeight)
      zoomedDimensions.width += this.scrollbarWidth_;
    return {
      horizontal: zoomedDimensions.width > this.window_.innerWidth,
      vertical: zoomedDimensions.height + this.topToolbarHeight_ >
          this.window_.innerHeight
    };
  },

  /**
   * Returns true if the document needs scrollbars at the current zoom level.
   * @return {Object} with 'x' and 'y' keys which map to bool values
   *     indicating if the horizontal and vertical scrollbars are needed
   *     respectively.
   */
  documentHasScrollbars: function() {
    return this.documentNeedsScrollbars_(this.zoom_);
  },

  /**
   * @private
   * Helper function called when the zoomed document size changes.
   */
  contentSizeChanged_: function() {
    var zoomedDimensions = this.getZoomedDocumentDimensions_(this.zoom_);
    if (zoomedDimensions) {
      this.sizer_.style.width = zoomedDimensions.width + 'px';
      this.sizer_.style.height = zoomedDimensions.height +
          this.topToolbarHeight_ + 'px';
    }
  },

  /**
   * @private
   * Called when the viewport should be updated.
   */
  updateViewport_: function() {
    this.viewportChangedCallback_();
  },

  /**
   * @private
   * Called when the viewport size changes.
   */
  resize_: function() {
    if (this.fittingType_ == Viewport.FittingType.FIT_TO_PAGE)
      this.fitToPageInternal_(false);
    else if (this.fittingType_ == Viewport.FittingType.FIT_TO_WIDTH)
      this.fitToWidth();
    else
      this.updateViewport_();
  },

  /**
   * @type {Object} the scroll position of the viewport.
   */
  get position() {
    return {
      x: this.window_.pageXOffset,
      y: this.window_.pageYOffset - this.topToolbarHeight_
    };
  },

  /**
   * Scroll the viewport to the specified position.
   * @type {Object} position the position to scroll to.
   */
  set position(position) {
    this.window_.scrollTo(position.x, position.y + this.topToolbarHeight_);
  },

  /**
   * @type {Object} the size of the viewport excluding scrollbars.
   */
  get size() {
    var needsScrollbars = this.documentNeedsScrollbars_(this.zoom_);
    var scrollbarWidth = needsScrollbars.vertical ? this.scrollbarWidth_ : 0;
    var scrollbarHeight = needsScrollbars.horizontal ? this.scrollbarWidth_ : 0;
    return {
      width: this.window_.innerWidth - scrollbarWidth,
      height: this.window_.innerHeight - scrollbarHeight
    };
  },

  /**
   * @type {number} the zoom level of the viewport.
   */
  get zoom() {
    return this.zoom_;
  },

  /**
   * @private
   * Used to wrap a function that might perform zooming on the viewport. This is
   * required so that we can notify the plugin that zooming is in progress
   * so that while zooming is taking place it can stop reacting to scroll events
   * from the viewport. This is to avoid flickering.
   */
  mightZoom_: function(f) {
    this.beforeZoomCallback_();
    this.allowedToChangeZoom_ = true;
    f();
    this.allowedToChangeZoom_ = false;
    this.afterZoomCallback_();
  },

  /**
   * @private
   * Sets the zoom of the viewport.
   * @param {number} newZoom the zoom level to zoom to.
   */
  setZoomInternal_: function(newZoom) {
    if (!this.allowedToChangeZoom_) {
      throw 'Called Viewport.setZoomInternal_ without calling ' +
            'Viewport.mightZoom_.';
    }
    // Record the scroll position (relative to the top-left of the window).
    var currentScrollPos = {
      x: this.position.x / this.zoom_,
      y: this.position.y / this.zoom_
    };
    this.zoom_ = newZoom;
    this.contentSizeChanged_();
    // Scroll to the scaled scroll position.
    this.position = {
      x: currentScrollPos.x * newZoom,
      y: currentScrollPos.y * newZoom
    };
  },

  /**
   * Sets the zoom to the given zoom level.
   * @param {number} newZoom the zoom level to zoom to.
   */
  setZoom: function(newZoom) {
    this.fittingType_ = Viewport.FittingType.NONE;
    newZoom = Math.max(Viewport.ZOOM_FACTOR_RANGE.min,
                       Math.min(newZoom, Viewport.ZOOM_FACTOR_RANGE.max));
    this.mightZoom_(function() {
      this.setZoomInternal_(newZoom);
      this.updateViewport_();
    }.bind(this));
  },

  /**
   * @type {number} the width of scrollbars in the viewport in pixels.
   */
  get scrollbarWidth() {
    return this.scrollbarWidth_;
  },

  /**
   * @type {Viewport.FittingType} the fitting type the viewport is currently in.
   */
  get fittingType() {
    return this.fittingType_;
  },

  /**
   * @private
   * @param {integer} y the y-coordinate to get the page at.
   * @return {integer} the index of a page overlapping the given y-coordinate.
   */
  getPageAtY_: function(y) {
    var min = 0;
    var max = this.pageDimensions_.length - 1;
    while (max >= min) {
      var page = Math.floor(min + ((max - min) / 2));
      // There might be a gap between the pages, in which case use the bottom
      // of the previous page as the top for finding the page.
      var top = 0;
      if (page > 0) {
        top = this.pageDimensions_[page - 1].y +
            this.pageDimensions_[page - 1].height;
      }
      var bottom = this.pageDimensions_[page].y +
          this.pageDimensions_[page].height;

      if (top <= y && bottom > y)
        return page;
      else if (top > y)
        max = page - 1;
      else
        min = page + 1;
    }
    return 0;
  },

  /**
   * Returns the page with the greatest proportion of its height in the current
   * viewport.
   * @return {int} the index of the most visible page.
   */
  getMostVisiblePage: function() {
    var firstVisiblePage = this.getPageAtY_(this.position.y / this.zoom_);
    if (firstVisiblePage == this.pageDimensions_.length - 1)
      return firstVisiblePage;

    var viewportRect = {
      x: this.position.x / this.zoom_,
      y: this.position.y / this.zoom_,
      width: this.size.width / this.zoom_,
      height: this.size.height / this.zoom_
    };
    var firstVisiblePageVisibility = getIntersectionHeight(
        this.pageDimensions_[firstVisiblePage], viewportRect) /
        this.pageDimensions_[firstVisiblePage].height;
    var nextPageVisibility = getIntersectionHeight(
        this.pageDimensions_[firstVisiblePage + 1], viewportRect) /
        this.pageDimensions_[firstVisiblePage + 1].height;
    if (nextPageVisibility > firstVisiblePageVisibility)
      return firstVisiblePage + 1;
    return firstVisiblePage;
  },

  /**
   * @private
   * Compute the zoom level for fit-to-page or fit-to-width. |pageDimensions| is
   * the dimensions for a given page and if |widthOnly| is true, it indicates
   * that fit-to-page zoom should be computed rather than fit-to-page.
   * @param {Object} pageDimensions the dimensions of a given page
   * @param {boolean} widthOnly a bool indicating whether fit-to-page or
   *     fit-to-width should be computed.
   * @return {number} the zoom to use
   */
  computeFittingZoom_: function(pageDimensions, widthOnly) {
    // First compute the zoom without scrollbars.
    var zoomWidth = this.window_.innerWidth / pageDimensions.width;
    var zoom;
    var zoomHeight;
    if (widthOnly) {
      zoom = zoomWidth;
    } else {
      zoomHeight = this.window_.innerHeight / pageDimensions.height;
      zoom = Math.min(zoomWidth, zoomHeight);
    }
    // Check if there needs to be any scrollbars.
    var needsScrollbars = this.documentNeedsScrollbars_(zoom);

    // If the document fits, just return the zoom.
    if (!needsScrollbars.horizontal && !needsScrollbars.vertical)
      return zoom;

    var zoomedDimensions = this.getZoomedDocumentDimensions_(zoom);

    // Check if adding a scrollbar will result in needing the other scrollbar.
    var scrollbarWidth = this.scrollbarWidth_;
    if (needsScrollbars.horizontal &&
        zoomedDimensions.height > this.window_.innerHeight - scrollbarWidth) {
      needsScrollbars.vertical = true;
    }
    if (needsScrollbars.vertical &&
        zoomedDimensions.width > this.window_.innerWidth - scrollbarWidth) {
      needsScrollbars.horizontal = true;
    }

    // Compute available window space.
    var windowWithScrollbars = {
      width: this.window_.innerWidth,
      height: this.window_.innerHeight
    };
    if (needsScrollbars.horizontal)
      windowWithScrollbars.height -= scrollbarWidth;
    if (needsScrollbars.vertical)
      windowWithScrollbars.width -= scrollbarWidth;

    // Recompute the zoom.
    zoomWidth = windowWithScrollbars.width / pageDimensions.width;
    if (widthOnly) {
      zoom = zoomWidth;
    } else {
      zoomHeight = windowWithScrollbars.height / pageDimensions.height;
      zoom = Math.min(zoomWidth, zoomHeight);
    }
    return zoom;
  },

  /**
   * Zoom the viewport so that the page-width consumes the entire viewport.
   */
  fitToWidth: function() {
    this.mightZoom_(function() {
      this.fittingType_ = Viewport.FittingType.FIT_TO_WIDTH;
      if (!this.documentDimensions_)
        return;
      // When computing fit-to-width, the maximum width of a page in the
      // document is used, which is equal to the size of the document width.
      this.setZoomInternal_(this.computeFittingZoom_(this.documentDimensions_,
                                                     true));
      var page = this.getMostVisiblePage();
      this.updateViewport_();
    }.bind(this));
  },

  /**
   * @private
   * Zoom the viewport so that a page consumes the entire viewport.
   * @param {boolean} scrollToTopOfPage Set to true if the viewport should be
   *     scrolled to the top of the current page. Set to false if the viewport
   *     should remain at the current scroll position.
   */
  fitToPageInternal_: function(scrollToTopOfPage) {
    this.mightZoom_(function() {
      this.fittingType_ = Viewport.FittingType.FIT_TO_PAGE;
      if (!this.documentDimensions_)
        return;
      var page = this.getMostVisiblePage();
      // Fit to the current page's height and the widest page's width.
      var dimensions = {
        width: this.documentDimensions_.width,
        height: this.pageDimensions_[page].height,
      };
      this.setZoomInternal_(this.computeFittingZoom_(dimensions, false));
      if (scrollToTopOfPage) {
        this.position = {
          x: 0,
          y: this.pageDimensions_[page].y * this.zoom_
        };
      }
      this.updateViewport_();
    }.bind(this));
  },

  /**
   * Zoom the viewport so that a page consumes the entire viewport. Also scrolls
   * the viewport to the top of the current page.
   */
  fitToPage: function() {
    this.fitToPageInternal_(true);
  },

  /**
   * Zoom out to the next predefined zoom level.
   */
  zoomOut: function() {
    this.mightZoom_(function() {
      this.fittingType_ = Viewport.FittingType.NONE;
      var nextZoom = Viewport.ZOOM_FACTORS[0];
      for (var i = 0; i < Viewport.ZOOM_FACTORS.length; i++) {
        if (Viewport.ZOOM_FACTORS[i] < this.zoom_)
          nextZoom = Viewport.ZOOM_FACTORS[i];
      }
      this.setZoomInternal_(nextZoom);
      this.updateViewport_();
    }.bind(this));
  },

  /**
   * Zoom in to the next predefined zoom level.
   */
  zoomIn: function() {
    this.mightZoom_(function() {
      this.fittingType_ = Viewport.FittingType.NONE;
      var nextZoom = Viewport.ZOOM_FACTORS[Viewport.ZOOM_FACTORS.length - 1];
      for (var i = Viewport.ZOOM_FACTORS.length - 1; i >= 0; i--) {
        if (Viewport.ZOOM_FACTORS[i] > this.zoom_)
          nextZoom = Viewport.ZOOM_FACTORS[i];
      }
      this.setZoomInternal_(nextZoom);
      this.updateViewport_();
    }.bind(this));
  },

  /**
   * Go to the given page index.
   * @param {number} page the index of the page to go to. zero-based.
   */
  goToPage: function(page) {
    this.mightZoom_(function() {
      if (this.pageDimensions_.length === 0)
        return;
      if (page < 0)
        page = 0;
      if (page >= this.pageDimensions_.length)
        page = this.pageDimensions_.length - 1;
      var dimensions = this.pageDimensions_[page];
      var toolbarOffset = 0;
      // Unless we're in fit to page mode, scroll above the page by
      // |this.topToolbarHeight_| so that the toolbar isn't covering it
      // initially.
      if (this.fittingType_ != Viewport.FittingType.FIT_TO_PAGE)
        toolbarOffset = this.topToolbarHeight_;
      this.position = {
        x: dimensions.x * this.zoom_,
        y: dimensions.y * this.zoom_ - toolbarOffset
      };
      this.updateViewport_();
    }.bind(this));
  },

  /**
   * Set the dimensions of the document.
   * @param {Object} documentDimensions the dimensions of the document
   */
  setDocumentDimensions: function(documentDimensions) {
    this.mightZoom_(function() {
      var initialDimensions = !this.documentDimensions_;
      this.documentDimensions_ = documentDimensions;
      this.pageDimensions_ = this.documentDimensions_.pageDimensions;
      if (initialDimensions) {
        this.setZoomInternal_(
            Math.min(this.defaultZoom_,
                     this.computeFittingZoom_(this.documentDimensions_, true)));
        this.position = {
          x: 0,
          y: -this.topToolbarHeight_
        };
      }
      this.contentSizeChanged_();
      this.resize_();
    }.bind(this));
  },

  /**
   * Get the coordinates of the page contents (excluding the page shadow)
   * relative to the screen.
   * @param {number} page the index of the page to get the rect for.
   * @return {Object} a rect representing the page in screen coordinates.
   */
  getPageScreenRect: function(page) {
    if (!this.documentDimensions_) {
      return {
        x: 0,
        y: 0,
        width: 0,
        height: 0
      };
    }
    if (page >= this.pageDimensions_.length)
      page = this.pageDimensions_.length - 1;

    var pageDimensions = this.pageDimensions_[page];

    // Compute the page dimensions minus the shadows.
    var insetDimensions = {
      x: pageDimensions.x + Viewport.PAGE_SHADOW.left,
      y: pageDimensions.y + Viewport.PAGE_SHADOW.top,
      width: pageDimensions.width - Viewport.PAGE_SHADOW.left -
          Viewport.PAGE_SHADOW.right,
      height: pageDimensions.height - Viewport.PAGE_SHADOW.top -
          Viewport.PAGE_SHADOW.bottom
    };

    // Compute the x-coordinate of the page within the document.
    // TODO(raymes): This should really be set when the PDF plugin passes the
    // page coordinates, but it isn't yet.
    var x = (this.documentDimensions_.width - pageDimensions.width) / 2 +
        Viewport.PAGE_SHADOW.left;
    // Compute the space on the left of the document if the document fits
    // completely in the screen.
    var spaceOnLeft = (this.size.width -
        this.documentDimensions_.width * this.zoom_) / 2;
    spaceOnLeft = Math.max(spaceOnLeft, 0);

    return {
      x: x * this.zoom_ + spaceOnLeft - this.window_.pageXOffset,
      y: insetDimensions.y * this.zoom_ - this.window_.pageYOffset,
      width: insetDimensions.width * this.zoom_,
      height: insetDimensions.height * this.zoom_
    };
  }
};
