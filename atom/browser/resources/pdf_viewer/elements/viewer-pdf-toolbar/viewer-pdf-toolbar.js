// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
(function() {
  Polymer({
    is: 'viewer-pdf-toolbar',

    behaviors: [
      Polymer.NeonAnimationRunnerBehavior
    ],

    properties: {
      /**
       * The current loading progress of the PDF document (0 - 100).
       */
      loadProgress: {
        type: Number,
        observer: 'loadProgressChanged'
      },

      /**
       * The title of the PDF document.
       */
      docTitle: String,

      /**
       * The number of the page being viewed (1-based).
       */
      pageNo: Number,

      /**
       * Tree of PDF bookmarks (or null if the document has no bookmarks).
       */
      bookmarks: {
        type: Object,
        value: null
      },

      /**
       * The number of pages in the PDF document.
       */
      docLength: Number,

      /**
       * Whether the toolbar is opened and visible.
       */
      opened: {
        type: Boolean,
        value: true
      },

      strings: Object,

      animationConfig: {
        value: function() {
          return {
            'entry': {
              name: 'transform-animation',
              node: this,
              transformFrom: 'translateY(-100%)',
              transformTo: 'translateY(0%)',
              timing: {
                easing: 'cubic-bezier(0, 0, 0.2, 1)',
                duration: 250
              }
            },
            'exit': {
              name: 'slide-up-animation',
              node: this,
              timing: {
                easing: 'cubic-bezier(0.4, 0, 1, 1)',
                duration: 250
              }
            }
          };
        }
      }
    },

    listeners: {
      'neon-animation-finish': '_onAnimationFinished'
    },

    _onAnimationFinished: function() {
      this.style.transform = this.opened ? 'none' : 'translateY(-100%)';
    },

    loadProgressChanged: function() {
      if (this.loadProgress >= 100) {
        this.$.pageselector.classList.toggle('invisible', false);
        this.$.buttons.classList.toggle('invisible', false);
        this.$.progress.style.opacity = 0;
      }
    },

    hide: function() {
      if (this.opened)
        this.toggleVisibility();
    },

    show: function() {
      if (!this.opened) {
        this.toggleVisibility();
      }
    },

    toggleVisibility: function() {
      this.opened = !this.opened;
      this.cancelAnimation();
      this.playAnimation(this.opened ? 'entry' : 'exit');
    },

    selectPageNumber: function() {
      this.$.pageselector.select();
    },

    shouldKeepOpen: function() {
      return this.$.bookmarks.dropdownOpen || this.loadProgress < 100 ||
          this.$.pageselector.isActive();
    },

    hideDropdowns: function() {
      if (this.$.bookmarks.dropdownOpen) {
        this.$.bookmarks.toggleDropdown();
        return true;
      }
      return false;
    },

    setDropdownLowerBound: function(lowerBound) {
      this.$.bookmarks.lowerBound = lowerBound;
    },

    rotateRight: function() {
      this.fire('rotate-right');
    },

    download: function() {
      this.fire('save');
    },

    print: function() {
      this.fire('print');
    }
  });
})();
