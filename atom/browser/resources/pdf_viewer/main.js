// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Global PDFViewer object, accessible for testing.
 * @type Object
 */
var viewer;


//(function() {
  /**
   * Stores any pending messages received which should be passed to the
   * PDFViewer when it is created.
   * @type Array
   */
  var pendingMessages = [];

  /**
   * Handles events that are received prior to the PDFViewer being created.
   * @param {Object} message A message event received.
   */
  function handleScriptingMessage(message) {
    pendingMessages.push(message);
  }

  /**
   * Initialize the global PDFViewer and pass any outstanding messages to it.
   * @param {Object} browserApi An object providing an API to the browser.
   */
  function initViewer(browserApi) {
    // PDFViewer will handle any messages after it is created.
    window.removeEventListener('message', handleScriptingMessage, false);
    viewer = new PDFViewer(browserApi);
    while (pendingMessages.length > 0)
      viewer.handleScriptingMessage(pendingMessages.shift());
  }

  /**
   * Entrypoint for starting the PDF viewer. This function obtains the browser
   * API for the PDF and constructs a PDFViewer object with it.
   */
  function main(streamURL, originalURL) {
    // Set up an event listener to catch scripting messages which are sent prior
    // to the PDFViewer being created.
    window.addEventListener('message', handleScriptingMessage, false);

    createBrowserApi(streamURL, originalURL).then(initViewer);
  };

  //main();
//})();
