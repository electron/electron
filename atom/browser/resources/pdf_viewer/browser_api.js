// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Returns a promise that will resolve to the default zoom factor.
 * @param {!Object} streamInfo The stream object pointing to the data contained
 *     in the PDF.
 * @return {Promise<number>} A promise that will resolve to the default zoom
 *     factor.
 */
function lookupDefaultZoom(streamInfo) {
  // Webviews don't run in tabs so |streamInfo.tabId| is -1 when running within
  // a webview.
  if (!chrome.tabs || streamInfo.tabId < 0)
    return Promise.resolve(1);

  return new Promise(function(resolve, reject) {
    chrome.tabs.getZoomSettings(streamInfo.tabId, function(zoomSettings) {
      resolve(zoomSettings.defaultZoomFactor);
    });
  });
}

/**
 * Returns a promise that will resolve to the initial zoom factor
 * upon starting the plugin. This may differ from the default zoom
 * if, for example, the page is zoomed before the plugin is run.
 * @param {!Object} streamInfo The stream object pointing to the data contained
 *     in the PDF.
 * @return {Promise<number>} A promise that will resolve to the initial zoom
 *     factor.
 */
function lookupInitialZoom(streamInfo) {
  // Webviews don't run in tabs so |streamInfo.tabId| is -1 when running within
  // a webview.
  if (!chrome.tabs || streamInfo.tabId < 0)
    return Promise.resolve(1);

  return new Promise(function(resolve, reject) {
    chrome.tabs.getZoom(streamInfo.tabId, resolve);
  });
}

/**
 * A class providing an interface to the browser.
 */
class BrowserApi {
  /**
   * @constructor
   * @param {!Object} streamInfo The stream object which points to the data
   *     contained in the PDF.
   * @param {number} defaultZoom The default browser zoom.
   * @param {number} initialZoom The initial browser zoom
   *     upon starting the plugin.
   * @param {boolean} manageZoom Whether to manage zoom.
   */
  constructor(streamInfo, defaultZoom, initialZoom, manageZoom) {
    this.streamInfo_ = streamInfo;
    this.defaultZoom_ = defaultZoom;
    this.initialZoom_ = initialZoom;
    this.manageZoom_ = manageZoom;
  }

  /**
   * Returns a promise to a BrowserApi.
   * @param {!Object} streamInfo The stream object pointing to the data
   *     contained in the PDF.
   * @param {boolean} manageZoom Whether to manage zoom.
   */
  static create(streamInfo, manageZoom) {
    return Promise.all([
      lookupDefaultZoom(streamInfo),
      lookupInitialZoom(streamInfo)
    ]).then(function(zoomFactors) {
      return new BrowserApi(
        streamInfo, zoomFactors[0], zoomFactors[1], manageZoom);
    });
  }

  /**
   * Returns the stream info pointing to the data contained in the PDF.
   * @return {Object} The stream info object.
   */
  getStreamInfo() {
    return this.streamInfo_;
  }

  /**
   * Aborts the stream.
   */
  abortStream() {
    if (chrome.mimeHandlerPrivate)
      chrome.mimeHandlerPrivate.abortStream();
  }

  /**
   * Sets the browser zoom.
   * @param {number} zoom The zoom factor to send to the browser.
   * @return {Promise} A promise that will be resolved when the browser zoom
   *     has been updated.
   */
  setZoom(zoom) {
    if (!this.manageZoom_)
      return Promise.resolve();
    return new Promise(function(resolve, reject) {
      chrome.tabs.setZoom(this.streamInfo_.tabId, zoom, resolve);
    }.bind(this));
  }

  /**
   * Returns the default browser zoom factor.
   * @return {number} The default browser zoom factor.
   */
  getDefaultZoom() {
    return this.defaultZoom_;
  }

  /**
   * Returns the initial browser zoom factor.
   * @return {number} The initial browser zoom factor.
   */
  getInitialZoom() {
    return this.initialZoom_;
  }

  /**
   * Adds an event listener to be notified when the browser zoom changes.
   * @param {function} listener The listener to be called with the new zoom
   *     factor.
   */
  addZoomEventListener(listener) {
    if (!this.manageZoom_)
      return;

    chrome.tabs.onZoomChange.addListener(function(zoomChangeInfo) {
      if (zoomChangeInfo.tabId != this.streamInfo_.tabId)
        return;
      listener(zoomChangeInfo.newZoomFactor);
    }.bind(this));
  }
};

/**
 * Creates a BrowserApi instance for an extension not running as a mime handler.
 * @return {Promise<BrowserApi>} A promise to a BrowserApi instance constructed
 *     from the URL.
 */
function createBrowserApi(streamURL, originalURL) {
  let streamInfo = {
    streamUrl: streamURL,
    originalUrl: originalURL,
    responseHeaders: {},
    embedded: window.parent != window,
    tabId: -1,
  };
  return new Promise(function(resolve, reject) {
    if (!chrome.tabs) {
      resolve();
      return;
    }
    //chrome.tabs.getCurrent(function(tab) {
      streamInfo.tabId = -1;
      resolve();
    //});
  }).then(function() { return BrowserApi.create(streamInfo, false); });
}
