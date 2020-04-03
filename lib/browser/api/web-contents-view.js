'use strict';

const electron = require('electron');

const { View } = electron;
const { WebContentsView } = process.electronBinding('web_contents_view');

Object.setPrototypeOf(WebContentsView.prototype, View.prototype);

module.exports = WebContentsView;
