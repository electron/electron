// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Creates a new Navigator for navigating to links inside or outside the PDF.
 * @param {string} originalUrl The original page URL.
 * @param {Object} viewport The viewport info of the page.
 * @param {Object} paramsParser The object for URL parsing.
 * @param {Function} navigateInCurrentTabCallback The Callback function that
 *    gets called when navigation happens in the current tab.
 * @param {Function} navigateInNewTabCallback The Callback function
 *    that gets called when navigation happens in the new tab.
 */
function Navigator(originalUrl,
                   viewport,
                   paramsParser,
                   navigateInCurrentTabCallback,
                   navigateInNewTabCallback) {
  this.originalUrl_ = originalUrl;
  this.viewport_ = viewport;
  this.paramsParser_ = paramsParser;
  this.navigateInCurrentTabCallback_ = navigateInCurrentTabCallback;
  this.navigateInNewTabCallback_ = navigateInNewTabCallback;
}

/**
 * Represents options when navigating to a new url. C++ counterpart of
 * the enum is in ui/base/window_open_disposition.h. This enum represents
 * the only values that are passed from Plugin.
 * @enum {number}
 */
Navigator.WindowOpenDisposition = {
  CURRENT_TAB: 1,
  NEW_FOREGROUND_TAB: 3,
  NEW_BACKGROUND_TAB: 4,
  NEW_WINDOW: 6,
  SAVE_TO_DISK: 7
};

Navigator.prototype = {
  /**
   * @private
   * Function to navigate to the given URL. This might involve navigating
   * within the PDF page or opening a new url (in the same tab or a new tab).
   * @param {string} url The URL to navigate to.
   * @param {number} disposition The window open disposition when
   *    navigating to the new URL.
   */
  navigate: function(url, disposition) {
    if (url.length == 0)
      return;

    // If |urlFragment| starts with '#', then it's for the same URL with a
    // different URL fragment.
    if (url.charAt(0) == '#') {
      // if '#' is already present in |originalUrl| then remove old fragment
      // and add new url fragment.
      var hashIndex = this.originalUrl_.search('#');
      if (hashIndex != -1)
        url = this.originalUrl_.substring(0, hashIndex) + url;
      else
        url = this.originalUrl_ + url;
    }

    // If there's no scheme, then take a guess at the scheme.
    if (url.indexOf('://') == -1 && url.indexOf('mailto:') == -1)
      url = this.guessUrlWithoutScheme_(url);

    if (!this.isValidUrl_(url))
      return;

    switch (disposition) {
      case Navigator.WindowOpenDisposition.CURRENT_TAB:
        this.paramsParser_.getViewportFromUrlParams(
            url, this.onViewportReceived_.bind(this));
        break;
      case Navigator.WindowOpenDisposition.NEW_BACKGROUND_TAB:
        this.navigateInNewTabCallback_(url, false);
        break;
      case Navigator.WindowOpenDisposition.NEW_FOREGROUND_TAB:
        this.navigateInNewTabCallback_(url, true);
        break;
      case Navigator.WindowOpenDisposition.NEW_WINDOW:
        // TODO(jaepark): Shift + left clicking a link in PDF should open the
        // link in a new window. See http://crbug.com/628057.
        this.paramsParser_.getViewportFromUrlParams(
            url, this.onViewportReceived_.bind(this));
        break;
      case Navigator.WindowOpenDisposition.SAVE_TO_DISK:
        // TODO(jaepark): Alt + left clicking a link in PDF should
        // download the link.
        this.paramsParser_.getViewportFromUrlParams(
            url, this.onViewportReceived_.bind(this));
        break;
      default:
        break;
    }
  },

  /**
   * @private
   * Called when the viewport position is received.
   * @param {Object} viewportPosition Dictionary containing the viewport
   *    position.
   */
  onViewportReceived_: function(viewportPosition) {
    var originalUrl = this.originalUrl_;
    var hashIndex = originalUrl.search('#');
    if (hashIndex != -1)
      originalUrl = originalUrl.substring(0, hashIndex);

    var newUrl = viewportPosition.url;
    hashIndex = newUrl.search('#');
    if (hashIndex != -1)
      newUrl = newUrl.substring(0, hashIndex);

    var pageNumber = viewportPosition.page;
    if (pageNumber != undefined && originalUrl == newUrl)
      this.viewport_.goToPage(pageNumber);
    else
      this.navigateInCurrentTabCallback_(viewportPosition.url);
  },

  /**
   * @private
   * Checks if the URL starts with a scheme and s not just a scheme.
   * @param {string} The input URL
   * @return {boolean} Whether the url is valid.
   */
  isValidUrl_: function(url) {
    // Make sure |url| starts with a valid scheme.
    if (url.indexOf('http://') != 0 &&
        url.indexOf('https://') != 0 &&
        url.indexOf('ftp://') != 0 &&
        url.indexOf('file://') != 0 &&
        url.indexOf('mailto:') != 0) {
      return false;
    }

    // Make sure |url| is not only a scheme.
    if (url == 'http://' ||
        url == 'https://' ||
        url == 'ftp://' ||
        url == 'file://' ||
        url == 'mailto:') {
      return false;
    }

    return true;
  },

  /**
   * @private
   * Attempt to figure out what a URL is when there is no scheme.
   * @param {string} The input URL
   * @return {string} The URL with a scheme or the original URL if it is not
   *     possible to determine the scheme.
   */
  guessUrlWithoutScheme_: function(url) {
    // If the original URL is mailto:, that does not make sense to start with,
    // and neither does adding |url| to it.
    // If the original URL is not a valid URL, this cannot make a valid URL.
    // In both cases, just bail out.
    if (this.originalUrl_.startsWith('mailto:') ||
        !this.isValidUrl_(this.originalUrl_)) {
      return url;
    }

    // Check for absolute paths.
    if (url.startsWith('/')) {
      var schemeEndIndex = this.originalUrl_.indexOf('://');
      var firstSlash = this.originalUrl_.indexOf('/', schemeEndIndex + 3);
      // e.g. http://www.foo.com/bar -> http://www.foo.com
      var domain = firstSlash != -1 ?
          this.originalUrl_.substr(0, firstSlash) : this.originalUrl_;
      return domain + url;
    }

    // Check for obvious relative paths.
    var isRelative = false;
    if (url.startsWith('.') || url.startsWith('\\'))
      isRelative = true;

    // In Adobe Acrobat Reader XI, it looks as though links with less than
    // 2 dot separators in the domain are considered relative links, and
    // those with 2 of more are considered http URLs. e.g.
    //
    // www.foo.com/bar -> http
    // foo.com/bar -> relative link
    if (!isRelative) {
      var domainSeparatorIndex = url.indexOf('/');
      var domainName = domainSeparatorIndex == -1 ?
          url : url.substr(0, domainSeparatorIndex);
      var domainDotCount = (domainName.match(/\./g) || []).length;
      if (domainDotCount < 2)
        isRelative = true;
    }

    if (isRelative) {
      var slashIndex = this.originalUrl_.lastIndexOf('/');
      var path = slashIndex != -1 ?
          this.originalUrl_.substr(0, slashIndex) : this.originalUrl_;
      return path + '/' + url;
    }

    return 'http://' + url;
  }
};
