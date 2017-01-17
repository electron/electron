// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Creates a new OpenPDFParamsParser. This parses the open pdf parameters
 * passed in the url to set initial viewport settings for opening the pdf.
 * @param {Object} getNamedDestinationsFunction The function called to fetch
 *     the page number for a named destination.
 */
function OpenPDFParamsParser(getNamedDestinationsFunction) {
  this.outstandingRequests_ = [];
  this.getNamedDestinationsFunction_ = getNamedDestinationsFunction;
}

OpenPDFParamsParser.prototype = {
  /**
   * @private
   * Parse zoom parameter of open PDF parameters. If this
   * parameter is passed while opening PDF then PDF should be opened
   * at the specified zoom level.
   * @param {number} zoom value.
   * @param {Object} viewportPosition to store zoom and position value.
   */
  parseZoomParam_: function(paramValue, viewportPosition) {
    var paramValueSplit = paramValue.split(',');
    if ((paramValueSplit.length != 1) && (paramValueSplit.length != 3))
      return;

    // User scale of 100 means zoom value of 100% i.e. zoom factor of 1.0.
    var zoomFactor = parseFloat(paramValueSplit[0]) / 100;
    if (isNaN(zoomFactor))
      return;

    // Handle #zoom=scale.
    if (paramValueSplit.length == 1) {
      viewportPosition['zoom'] = zoomFactor;
      return;
    }

    // Handle #zoom=scale,left,top.
    var position = {x: parseFloat(paramValueSplit[1]),
                    y: parseFloat(paramValueSplit[2])};
    viewportPosition['position'] = position;
    viewportPosition['zoom'] = zoomFactor;
  },

  /**
   * Parse the parameters encoded in the fragment of a URL into a dictionary.
   * @private
   * @param {string} url to parse
   * @return {Object} Key-value pairs of URL parameters
   */
  parseUrlParams_: function(url) {
    var params = {};

    var paramIndex = url.search('#');
    if (paramIndex == -1)
      return params;

    var paramTokens = url.substring(paramIndex + 1).split('&');
    if ((paramTokens.length == 1) && (paramTokens[0].search('=') == -1)) {
      // Handle the case of http://foo.com/bar#NAMEDDEST. This is not
      // explicitly mentioned except by example in the Adobe
      // "PDF Open Parameters" document.
      params['nameddest'] = paramTokens[0];
      return params;
    }

    for (var i = 0; i < paramTokens.length; ++i) {
      var keyValueSplit = paramTokens[i].split('=');
      if (keyValueSplit.length != 2)
        continue;
      params[keyValueSplit[0]] = keyValueSplit[1];
    }

    return params;
  },

  /**
   * Parse PDF url parameters used for controlling the state of UI. These need
   * to be available when the UI is being initialized, rather than when the PDF
   * is finished loading.
   * @param {string} url that needs to be parsed.
   * @return {Object} parsed url parameters.
   */
  getUiUrlParams: function(url) {
    var params = this.parseUrlParams_(url);
    var uiParams = {toolbar: true};

    if ('toolbar' in params && params['toolbar'] == 0)
      uiParams.toolbar = false;

    return uiParams;
  },

  /**
   * Parse PDF url parameters. These parameters are mentioned in the url
   * and specify actions to be performed when opening pdf files.
   * See http://www.adobe.com/content/dam/Adobe/en/devnet/acrobat/
   * pdfs/pdf_open_parameters.pdf for details.
   * @param {string} url that needs to be parsed.
   * @param {Function} callback function to be called with viewport info.
   */
  getViewportFromUrlParams: function(url, callback) {
    var viewportPosition = {};
    viewportPosition['url'] = url;

    var paramsDictionary = this.parseUrlParams_(url);

    if ('page' in paramsDictionary) {
      // |pageNumber| is 1-based, but goToPage() take a zero-based page number.
      var pageNumber = parseInt(paramsDictionary['page']);
      if (!isNaN(pageNumber) && pageNumber > 0)
        viewportPosition['page'] = pageNumber - 1;
    }

    if ('zoom' in paramsDictionary)
      this.parseZoomParam_(paramsDictionary['zoom'], viewportPosition);

    if (viewportPosition.page === undefined &&
        'nameddest' in paramsDictionary) {
      this.outstandingRequests_.push({
        callback: callback,
        viewportPosition: viewportPosition
      });
      this.getNamedDestinationsFunction_(paramsDictionary['nameddest']);
    } else {
      callback(viewportPosition);
    }
  },

  /**
   * This is called when a named destination is received and the page number
   * corresponding to the request for which a named destination is passed.
   * @param {number} pageNumber The page corresponding to the named destination
   *    requested.
   */
  onNamedDestinationReceived: function(pageNumber) {
    var outstandingRequest = this.outstandingRequests_.shift();
    if (pageNumber != -1)
      outstandingRequest.viewportPosition.page = pageNumber;
    outstandingRequest.callback(outstandingRequest.viewportPosition);
  },
};
