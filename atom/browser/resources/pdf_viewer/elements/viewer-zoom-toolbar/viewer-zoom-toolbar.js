// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var FIT_TO_PAGE = 0;
  var FIT_TO_WIDTH = 1;

  Polymer({
    is: 'viewer-zoom-toolbar',

    properties: {
      strings: {
        type: Object,
        observer: 'updateTooltips_'
      },

      visible_: {
        type: Boolean,
        value: true
      }
    },

    isVisible: function() {
      return this.visible_;
    },

    /**
     * Change button tooltips to match any changes to localized strings.
     */
    updateTooltips_: function() {
      this.$['fit-button'].tooltips = [
          this.strings.tooltipFitToPage,
          this.strings.tooltipFitToWidth
      ];
      this.$['zoom-in-button'].tooltips = [this.strings.tooltipZoomIn];
      this.$['zoom-out-button'].tooltips = [this.strings.tooltipZoomOut];
    },

    fitToggle: function() {
      if (this.$['fit-button'].activeIndex == FIT_TO_WIDTH)
        this.fire('fit-to-width');
      else
        this.fire('fit-to-page');
    },

    zoomIn: function() {
      this.fire('zoom-in');
    },

    zoomOut: function() {
      this.fire('zoom-out');
    },

    show: function() {
      if (!this.visible_) {
        this.visible_ = true;
        this.$['fit-button'].show();
        this.$['zoom-in-button'].show();
        this.$['zoom-out-button'].show();
      }
    },

    hide: function() {
      if (this.visible_) {
        this.visible_ = false;
        this.$['fit-button'].hide();
        this.$['zoom-in-button'].hide();
        this.$['zoom-out-button'].hide();
      }
    },
  });

})();
