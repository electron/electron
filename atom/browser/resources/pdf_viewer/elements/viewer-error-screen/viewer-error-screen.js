// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'viewer-error-screen',
  properties: {
    reloadFn: {
      type: Object,
      value: null,
      observer: 'reloadFnChanged_'
    },

    strings: Object,
  },

  reloadFnChanged_: function() {
    // The default margins in paper-dialog don't work well with hiding/showing
    // the .buttons div. We need to manually manage the bottom margin to get
    // around this.
    if (this.reloadFn)
      this.$['load-failed-message'].classList.remove('last-item');
    else
      this.$['load-failed-message'].classList.add('last-item');
  },

  show: function() {
    this.$.dialog.open();
  },

  reload: function() {
    if (this.reloadFn)
      this.reloadFn();
  }
});
