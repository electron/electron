// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'viewer-password-screen',

  properties: {
    invalid: Boolean,

    active: {
      type: Boolean,
      value: false,
      observer: 'activeChanged'
    },

    strings: Object,
  },

  ready: function() {
    this.activeChanged();
  },

  accept: function() {
    this.active = false;
  },

  deny: function() {
    this.$.password.disabled = false;
    this.$.submit.disabled = false;
    this.invalid = true;
    this.$.password.focus();
    this.$.password.select();
  },

  handleKey: function(e) {
    if (e.keyCode == 13)
      this.submit();
  },

  submit: function() {
    if (this.$.password.value.length == 0)
      return;
    this.$.password.disabled = true;
    this.$.submit.disabled = true;
    this.fire('password-submitted', {password: this.$.password.value});
  },

  activeChanged: function() {
    if (this.active) {
      this.$.dialog.open();
      this.$.password.focus();
    } else {
      this.$.dialog.close();
    }
  }
});
