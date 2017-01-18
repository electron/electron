// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * @return {number} Width of a scrollbar in pixels
 */
function getScrollbarWidth() {
  var div = document.createElement('div');
  div.style.visibility = 'hidden';
  div.style.overflow = 'scroll';
  div.style.width = '50px';
  div.style.height = '50px';
  div.style.position = 'absolute';
  document.body.appendChild(div);
  var result = div.offsetWidth - div.clientWidth;
  div.parentNode.removeChild(div);
  return result;
}

/**
 * Return the filename component of a URL, percent decoded if possible.
 * @param {string} url The URL to get the filename from.
 * @return {string} The filename component.
 */
function getFilenameFromURL(url) {
  // Ignore the query and fragment.
  var mainUrl = url.split(/#|\?/)[0];
  var components = mainUrl.split(/\/|\\/);
  var filename = components[components.length - 1];
  try {
    return decodeURIComponent(filename);
  } catch (e) {
    if (e instanceof URIError)
      return filename;
    throw e;
  }
}

/**
 * Called when navigation happens in the current tab.
 * @param {string} url The url to be opened in the current tab.
 */
function onNavigateInCurrentTab(url) {
  window.location.href = url;
}

/**
 * Called when navigation happens in the new tab.
 * @param {string} url The url to be opened in the new tab.
 */
function onNavigateInNewTab(url) {
  window.open(url);
}

/**
 * Whether keydown events should currently be ignored. Events are ignored when
 * an editable element has focus, to allow for proper editing controls.
 * @param {HTMLElement} activeElement The currently selected DOM node.
 * @return {boolean} True if keydown events should be ignored.
 */
function shouldIgnoreKeyEvents(activeElement) {
  while (activeElement.shadowRoot != null &&
         activeElement.shadowRoot.activeElement != null) {
    activeElement = activeElement.shadowRoot.activeElement;
  }

  return (activeElement.isContentEditable ||
          activeElement.tagName == 'INPUT' ||
          activeElement.tagName == 'TEXTAREA');
}

/**
 * The minimum number of pixels to offset the toolbar by from the bottom and
 * right side of the screen.
 */
PDFViewer.MIN_TOOLBAR_OFFSET = 15;

/**
 * The height of the toolbar along the top of the page. The document will be
 * shifted down by this much in the viewport.
 */
PDFViewer.MATERIAL_TOOLBAR_HEIGHT = 56;

/**
 * Minimum height for the material toolbar to show (px). Should match the media
 * query in index-material.css. If the window is smaller than this at load,
 * leave no space for the toolbar.
 */
PDFViewer.TOOLBAR_WINDOW_MIN_HEIGHT = 250;

/**
 * The light-gray background color used for print preview.
 */
PDFViewer.LIGHT_BACKGROUND_COLOR = '0xFFCCCCCC';

/**
 * The dark-gray background color used for the regular viewer.
 */
PDFViewer.DARK_BACKGROUND_COLOR = '0xFF525659';

/**
 * Creates a new PDFViewer. There should only be one of these objects per
 * document.
 * @constructor
 * @param {!BrowserApi} browserApi An object providing an API to the browser.
 */
function PDFViewer(browserApi) {
  this.browserApi_ = browserApi;
  this.originalUrl_ = this.browserApi_.getStreamInfo().originalUrl;
  this.loadState_ = LoadState.LOADING;
  this.parentWindow_ = null;
  this.parentOrigin_ = null;
  this.isFormFieldFocused_ = false;

  this.delayedScriptingMessages_ = [];

  this.isPrintPreview_ = this.originalUrl_.indexOf('chrome://print') == 0;

  // Parse open pdf parameters.
  this.paramsParser_ =
      new OpenPDFParamsParser(this.getNamedDestination_.bind(this));
  var toolbarEnabled =
      this.paramsParser_.getUiUrlParams(this.originalUrl_).toolbar &&
      !this.isPrintPreview_;

  // The sizer element is placed behind the plugin element to cause scrollbars
  // to be displayed in the window. It is sized according to the document size
  // of the pdf and zoom level.
  this.sizer_ = $('sizer');
  if (this.isPrintPreview_)
    this.pageIndicator_ = $('page-indicator');
  this.passwordScreen_ = $('password-screen');
  this.passwordScreen_.addEventListener('password-submitted',
                                        this.onPasswordSubmitted_.bind(this));
  this.errorScreen_ = $('error-screen');
  // Can only reload if we are in a normal tab.
  if (!this.browserApi_.getStreamInfo().embedded) {
    this.errorScreen_.reloadFn = function() {
      chrome.send('reload');
    };
  }

  // Create the viewport.
  var shortWindow = window.innerHeight < PDFViewer.TOOLBAR_WINDOW_MIN_HEIGHT;
  var topToolbarHeight =
      (toolbarEnabled) ? PDFViewer.MATERIAL_TOOLBAR_HEIGHT : 0;
  this.viewport_ = new Viewport(window,
                                this.sizer_,
                                this.viewportChanged_.bind(this),
                                this.beforeZoom_.bind(this),
                                this.afterZoom_.bind(this),
                                getScrollbarWidth(),
                                this.browserApi_.getDefaultZoom(),
                                topToolbarHeight);

  // Create the plugin object dynamically so we can set its src. The plugin
  // element is sized to fill the entire window and is set to be fixed
  // positioning, acting as a viewport. The plugin renders into this viewport
  // according to the scroll position of the window.
  this.plugin_ = document.createElement('embed');
  // NOTE: The plugin's 'id' field must be set to 'plugin' since
  // chrome/renderer/printing/print_web_view_helper.cc actually references it.
  this.plugin_.id = 'plugin';
  this.plugin_.type = 'application/x-google-chrome-pdf';
  this.plugin_.addEventListener('message', this.handlePluginMessage_.bind(this),
                                false);

  // Handle scripting messages from outside the extension that wish to interact
  // with it. We also send a message indicating that extension has loaded and
  // is ready to receive messages.
  window.addEventListener('message', this.handleScriptingMessage.bind(this),
                          false);

  this.plugin_.setAttribute('src', this.originalUrl_);
  this.plugin_.setAttribute('stream-url',
                            this.browserApi_.getStreamInfo().streamUrl);
  var headers = '';
  for (var header in this.browserApi_.getStreamInfo().responseHeaders) {
    headers += header + ': ' +
        this.browserApi_.getStreamInfo().responseHeaders[header] + '\n';
  }
  this.plugin_.setAttribute('headers', headers);

  var backgroundColor = PDFViewer.DARK_BACKGROUND_COLOR;
  this.plugin_.setAttribute('background-color', backgroundColor);
  this.plugin_.setAttribute('top-toolbar-height', topToolbarHeight);

  if (!this.browserApi_.getStreamInfo().embedded)
    this.plugin_.setAttribute('full-frame', '');
  this.sizer_.appendChild(this.plugin_);

  // Setup the button event listeners.
  this.zoomToolbar_ = $('zoom-toolbar');
  this.zoomToolbar_.addEventListener('fit-to-width',
      this.viewport_.fitToWidth.bind(this.viewport_));
  this.zoomToolbar_.addEventListener('fit-to-page',
      this.fitToPage_.bind(this));
  this.zoomToolbar_.addEventListener('zoom-in',
      this.viewport_.zoomIn.bind(this.viewport_));
  this.zoomToolbar_.addEventListener('zoom-out',
      this.viewport_.zoomOut.bind(this.viewport_));

  if (toolbarEnabled) {
    this.toolbar_ = $('toolbar');
    this.toolbar_.hidden = false;
    this.toolbar_.addEventListener('save', this.save_.bind(this));
    this.toolbar_.addEventListener('print', this.print_.bind(this));
    this.toolbar_.addEventListener('rotate-right',
        this.rotateClockwise_.bind(this));
    // Must attach to mouseup on the plugin element, since it eats mousedown
    // and click events.
    this.plugin_.addEventListener('mouseup',
        this.toolbar_.hideDropdowns.bind(this.toolbar_));

    this.toolbar_.docTitle = getFilenameFromURL(this.originalUrl_);
  }

  document.body.addEventListener('change-page', function(e) {
    this.viewport_.goToPage(e.detail.page);
  }.bind(this));

  document.body.addEventListener('navigate', function(e) {
    var disposition =
        e.detail.newtab ? Navigator.WindowOpenDisposition.NEW_BACKGROUND_TAB :
                          Navigator.WindowOpenDisposition.CURRENT_TAB;
    this.navigator_.navigate(e.detail.uri, disposition);
  }.bind(this));

  this.toolbarManager_ =
      new ToolbarManager(window, this.toolbar_, this.zoomToolbar_);

  // Set up the ZoomManager.
  this.zoomManager_ = new ZoomManager(
      this.viewport_, this.browserApi_.setZoom.bind(this.browserApi_),
      this.browserApi_.getInitialZoom());
  this.browserApi_.addZoomEventListener(
      this.zoomManager_.onBrowserZoomChange.bind(this.zoomManager_));

  // Setup the keyboard event listener.
  document.addEventListener('keydown', this.handleKeyEvent_.bind(this));
  document.addEventListener('mousemove', this.handleMouseEvent_.bind(this));
  document.addEventListener('mouseout', this.handleMouseEvent_.bind(this));

  this.navigator_ = new Navigator(this.originalUrl_,
                                  this.viewport_, this.paramsParser_,
                                  onNavigateInCurrentTab,
                                  onNavigateInNewTab);
  this.viewportScroller_ =
      new ViewportScroller(this.viewport_, this.plugin_, window);

  // Request translated strings.
  //chrome.resourcesPrivate.getStrings('pdf', this.handleStrings_.bind(this));
}

PDFViewer.prototype = {
  /**
   * @private
   * Handle key events. These may come from the user directly or via the
   * scripting API.
   * @param {KeyboardEvent} e the event to handle.
   */
  handleKeyEvent_: function(e) {
    var position = this.viewport_.position;
    // Certain scroll events may be sent from outside of the extension.
    var fromScriptingAPI = e.fromScriptingAPI;

    if (shouldIgnoreKeyEvents(document.activeElement) || e.defaultPrevented)
      return;

    this.toolbarManager_.hideToolbarsAfterTimeout(e);

    var pageUpHandler = function() {
      // Go to the previous page if we are fit-to-page.
      if (this.viewport_.fittingType == Viewport.FittingType.FIT_TO_PAGE) {
        this.viewport_.goToPage(this.viewport_.getMostVisiblePage() - 1);
        // Since we do the movement of the page.
        e.preventDefault();
      } else if (fromScriptingAPI) {
        position.y -= this.viewport.size.height;
        this.viewport.position = position;
      }
    }.bind(this);
    var pageDownHandler = function() {
      // Go to the next page if we are fit-to-page.
      if (this.viewport_.fittingType == Viewport.FittingType.FIT_TO_PAGE) {
        this.viewport_.goToPage(this.viewport_.getMostVisiblePage() + 1);
        // Since we do the movement of the page.
        e.preventDefault();
      } else if (fromScriptingAPI) {
        position.y += this.viewport.size.height;
        this.viewport.position = position;
      }
    }.bind(this);

    switch (e.keyCode) {
      case 9:  // Tab key.
        this.toolbarManager_.showToolbarsForKeyboardNavigation();
        return;
      case 27:  // Escape key.
        if (!this.isPrintPreview_) {
          this.toolbarManager_.hideSingleToolbarLayer();
          return;
        }
        break;  // Ensure escape falls through to the print-preview handler.
      case 32:  // Space key.
        if (e.shiftKey)
          pageUpHandler();
        else
          pageDownHandler();
        return;
      case 33:  // Page up key.
        pageUpHandler();
        return;
      case 34:  // Page down key.
        pageDownHandler();
        return;
      case 37:  // Left arrow key.
        if (!(e.altKey || e.ctrlKey || e.metaKey || e.shiftKey)) {
          // Go to the previous page if there are no horizontal scrollbars and
          // no form field is focused.
          if (!(this.viewport_.documentHasScrollbars().horizontal ||
                this.isFormFieldFocused_)) {
            this.viewport_.goToPage(this.viewport_.getMostVisiblePage() - 1);
            // Since we do the movement of the page.
            e.preventDefault();
          } else if (fromScriptingAPI) {
            position.x -= Viewport.SCROLL_INCREMENT;
            this.viewport.position = position;
          }
        }
        return;
      case 38:  // Up arrow key.
        if (fromScriptingAPI) {
          position.y -= Viewport.SCROLL_INCREMENT;
          this.viewport.position = position;
        }
        return;
      case 39:  // Right arrow key.
        if (!(e.altKey || e.ctrlKey || e.metaKey || e.shiftKey)) {
          // Go to the next page if there are no horizontal scrollbars and no
          // form field is focused.
          if (!(this.viewport_.documentHasScrollbars().horizontal ||
                this.isFormFieldFocused_)) {
            this.viewport_.goToPage(this.viewport_.getMostVisiblePage() + 1);
            // Since we do the movement of the page.
            e.preventDefault();
          } else if (fromScriptingAPI) {
            position.x += Viewport.SCROLL_INCREMENT;
            this.viewport.position = position;
          }
        }
        return;
      case 40:  // Down arrow key.
        if (fromScriptingAPI) {
          position.y += Viewport.SCROLL_INCREMENT;
          this.viewport.position = position;
        }
        return;
      case 65:  // a key.
        if (e.ctrlKey || e.metaKey) {
          this.plugin_.postMessage({
            type: 'selectAll'
          });
          // Since we do selection ourselves.
          e.preventDefault();
        }
        return;
      case 71: // g key.
        if (this.toolbar_ && (e.ctrlKey || e.metaKey) && e.altKey) {
          this.toolbarManager_.showToolbars();
          this.toolbar_.selectPageNumber();
        }
        return;
      case 219:  // left bracket.
        if (e.ctrlKey)
          this.rotateCounterClockwise_();
        return;
      case 221:  // right bracket.
        if (e.ctrlKey)
          this.rotateClockwise_();
        return;
    }

    // Give print preview a chance to handle the key event.
    if (!fromScriptingAPI && this.isPrintPreview_) {
      this.sendScriptingMessage_({
        type: 'sendKeyEvent',
        keyEvent: SerializeKeyEvent(e)
      });
    } else {
      // Show toolbars as a fallback.
      if (!(e.shiftKey || e.ctrlKey || e.altKey))
        this.toolbarManager_.showToolbars();
    }
  },

  handleMouseEvent_: function(e) {
    if (e.type == 'mousemove')
      this.toolbarManager_.handleMouseMove(e);
    else if (e.type == 'mouseout')
      this.toolbarManager_.hideToolbarsForMouseOut();
  },

  /**
   * @private
   * Rotate the plugin clockwise.
   */
  rotateClockwise_: function() {
    this.plugin_.postMessage({
      type: 'rotateClockwise'
    });
  },

  /**
   * @private
   * Rotate the plugin counter-clockwise.
   */
  rotateCounterClockwise_: function() {
    this.plugin_.postMessage({
      type: 'rotateCounterclockwise'
    });
  },

  fitToPage_: function() {
    this.viewport_.fitToPage();
    this.toolbarManager_.forceHideTopToolbar();
  },

  /**
   * @private
   * Notify the plugin to print.
   */
  print_: function() {
    this.plugin_.postMessage({
      type: 'print'
    });
  },

  /**
   * @private
   * Notify the plugin to save.
   */
  save_: function() {
    this.plugin_.postMessage({
      type: 'save'
    });
  },

  /**
   * Fetches the page number corresponding to the given named destination from
   * the plugin.
   * @param {string} name The namedDestination to fetch page number from plugin.
   */
  getNamedDestination_: function(name) {
    this.plugin_.postMessage({
      type: 'getNamedDestination',
      namedDestination: name
    });
  },

  /**
   * @private
   * Sends a 'documentLoaded' message to the PDFScriptingAPI if the document has
   * finished loading.
   */
  sendDocumentLoadedMessage_: function() {
    if (this.loadState_ == LoadState.LOADING)
      return;
    this.sendScriptingMessage_({
      type: 'documentLoaded',
      load_state: this.loadState_
    });
  },

  /**
   * @private
   * Handle open pdf parameters. This function updates the viewport as per
   * the parameters mentioned in the url while opening pdf. The order is
   * important as later actions can override the effects of previous actions.
   * @param {Object} viewportPosition The initial position of the viewport to be
   *     displayed.
   */
  handleURLParams_: function(viewportPosition) {
    if (viewportPosition.page != undefined)
      this.viewport_.goToPage(viewportPosition.page);
    if (viewportPosition.position) {
      // Make sure we don't cancel effect of page parameter.
      this.viewport_.position = {
        x: this.viewport_.position.x + viewportPosition.position.x,
        y: this.viewport_.position.y + viewportPosition.position.y
      };
    }
    if (viewportPosition.zoom)
      this.viewport_.setZoom(viewportPosition.zoom);
  },

  /**
   * @private
   * Update the loading progress of the document in response to a progress
   * message being received from the plugin.
   * @param {number} progress the progress as a percentage.
   */
  updateProgress_: function(progress) {
    if (this.toolbar_)
      this.toolbar_.loadProgress = progress;

    if (progress == -1) {
      // Document load failed.
      this.errorScreen_.show();
      this.sizer_.style.display = 'none';
      if (this.passwordScreen_.active) {
        this.passwordScreen_.deny();
        this.passwordScreen_.active = false;
      }
      this.loadState_ = LoadState.FAILED;
      this.sendDocumentLoadedMessage_();
    } else if (progress == 100) {
      // Document load complete.
      if (this.lastViewportPosition_)
        this.viewport_.position = this.lastViewportPosition_;
      this.paramsParser_.getViewportFromUrlParams(
          this.originalUrl_,
          this.handleURLParams_.bind(this));
      this.loadState_ = LoadState.SUCCESS;
      this.sendDocumentLoadedMessage_();
      while (this.delayedScriptingMessages_.length > 0)
        this.handleScriptingMessage(this.delayedScriptingMessages_.shift());

      this.toolbarManager_.hideToolbarsAfterTimeout();
    }
  },

  /**
   * @private
   * Load a dictionary of translated strings into the UI. Used as a callback for
   * chrome.resourcesPrivate.
   * @param {Object} strings Dictionary of translated strings
   */
  handleStrings_: function(strings) {
    document.documentElement.dir = strings.textdirection;
    document.documentElement.lang = strings.language;

    $('toolbar').strings = strings;
    $('zoom-toolbar').strings = strings;
    $('password-screen').strings = strings;
    $('error-screen').strings = strings;
  },

  /**
   * @private
   * An event handler for handling password-submitted events. These are fired
   * when an event is entered into the password screen.
   * @param {Object} event a password-submitted event.
   */
  onPasswordSubmitted_: function(event) {
    this.plugin_.postMessage({
      type: 'getPasswordComplete',
      password: event.detail.password
    });
  },

  /**
   * @private
   * An event handler for handling message events received from the plugin.
   * @param {MessageObject} message a message event.
   */
  handlePluginMessage_: function(message) {
    switch (message.data.type.toString()) {
      case 'documentDimensions':
        this.documentDimensions_ = message.data;
        this.viewport_.setDocumentDimensions(this.documentDimensions_);
        // If we received the document dimensions, the password was good so we
        // can dismiss the password screen.
        if (this.passwordScreen_.active)
          this.passwordScreen_.accept();

        if (this.pageIndicator_)
          this.pageIndicator_.initialFadeIn();

        if (this.toolbar_) {
          this.toolbar_.docLength =
              this.documentDimensions_.pageDimensions.length;
        }
        break;
      case 'email':
        var href = 'mailto:' + message.data.to + '?cc=' + message.data.cc +
            '&bcc=' + message.data.bcc + '&subject=' + message.data.subject +
            '&body=' + message.data.body;
        window.location.href = href;
        break;
      case 'getPassword':
        // If the password screen isn't up, put it up. Otherwise we're
        // responding to an incorrect password so deny it.
        if (!this.passwordScreen_.active)
          this.passwordScreen_.active = true;
        else
          this.passwordScreen_.deny();
        break;
      case 'getSelectedTextReply':
        this.sendScriptingMessage_(message.data);
        break;
      case 'goToPage':
        this.viewport_.goToPage(message.data.page);
        break;
      case 'loadProgress':
        this.updateProgress_(message.data.progress);
        break;
      case 'navigate':
        // If in print preview, always open a new tab.
        if (this.isPrintPreview_) {
          this.navigator_.navigate(
              message.data.url,
              Navigator.WindowOpenDisposition.NEW_BACKGROUND_TAB);
        } else {
          this.navigator_.navigate(message.data.url, message.data.disposition);
        }
        break;
      case 'setScrollPosition':
        var position = this.viewport_.position;
        if (message.data.x !== undefined)
          position.x = message.data.x;
        if (message.data.y !== undefined)
          position.y = message.data.y;
        this.viewport_.position = position;
        break;
      case 'cancelStreamUrl':
        //chrome.mimeHandlerPrivate.abortStream();
        break;
      case 'metadata':
        if (message.data.title) {
          document.title = message.data.title;
        } else {
          document.title = getFilenameFromURL(this.originalUrl_);
        }
        this.bookmarks_ = message.data.bookmarks;
        if (this.toolbar_) {
          this.toolbar_.docTitle = document.title;
          this.toolbar_.bookmarks = this.bookmarks;
        }
        break;
      case 'setIsSelecting':
        this.viewportScroller_.setEnableScrolling(message.data.isSelecting);
        break;
      case 'getNamedDestinationReply':
        this.paramsParser_.onNamedDestinationReceived(message.data.pageNumber);
        break;
      case 'formFocusChange':
        this.isFormFieldFocused_ = message.data.focused;
        break;
    }
  },

  /**
   * @private
   * A callback that's called before the zoom changes. Notify the plugin to stop
   * reacting to scroll events while zoom is taking place to avoid flickering.
   */
  beforeZoom_: function() {
    this.plugin_.postMessage({
      type: 'stopScrolling'
    });
  },

  /**
   * @private
   * A callback that's called after the zoom changes. Notify the plugin of the
   * zoom change and to continue reacting to scroll events.
   */
  afterZoom_: function() {
    var position = this.viewport_.position;
    var zoom = this.viewport_.zoom;
    this.plugin_.postMessage({
      type: 'viewport',
      zoom: zoom,
      xOffset: position.x,
      yOffset: position.y
    });
    this.zoomManager_.onPdfZoomChange();
  },

  /**
   * @private
   * A callback that's called after the viewport changes.
   */
  viewportChanged_: function() {
    if (!this.documentDimensions_)
      return;

    // Offset the toolbar position so that it doesn't move if scrollbars appear.
    var hasScrollbars = this.viewport_.documentHasScrollbars();
    var scrollbarWidth = this.viewport_.scrollbarWidth;
    var verticalScrollbarWidth = hasScrollbars.vertical ? scrollbarWidth : 0;
    var horizontalScrollbarWidth =
        hasScrollbars.horizontal ? scrollbarWidth : 0;

    // Shift the zoom toolbar to the left by half a scrollbar width. This
    // gives a compromise: if there is no scrollbar visible then the toolbar
    // will be half a scrollbar width further left than the spec but if there
    // is a scrollbar visible it will be half a scrollbar width further right
    // than the spec. In RTL layout, the zoom toolbar is on the left side, but
    // the scrollbar is still on the right, so this is not necessary.
    if (!isRTL()) {
      this.zoomToolbar_.style.right = -verticalScrollbarWidth +
          (scrollbarWidth / 2) + 'px';
    }
    // Having a horizontal scrollbar is much rarer so we don't offset the
    // toolbar from the bottom any more than what the spec says. This means
    // that when there is a scrollbar visible, it will be a full scrollbar
    // width closer to the bottom of the screen than usual, but this is ok.
    this.zoomToolbar_.style.bottom = -horizontalScrollbarWidth + 'px';

    // Update the page indicator.
    var visiblePage = this.viewport_.getMostVisiblePage();

    if (this.toolbar_)
      this.toolbar_.pageNo = visiblePage + 1;

    // TODO(raymes): Give pageIndicator_ the same API as toolbar_.
    if (this.pageIndicator_) {
      this.pageIndicator_.index = visiblePage;
      if (this.documentDimensions_.pageDimensions.length > 1 &&
          hasScrollbars.vertical) {
        this.pageIndicator_.style.visibility = 'visible';
      } else {
        this.pageIndicator_.style.visibility = 'hidden';
      }
    }

    var visiblePageDimensions = this.viewport_.getPageScreenRect(visiblePage);
    var size = this.viewport_.size;
    this.sendScriptingMessage_({
      type: 'viewport',
      pageX: visiblePageDimensions.x,
      pageY: visiblePageDimensions.y,
      pageWidth: visiblePageDimensions.width,
      viewportWidth: size.width,
      viewportHeight: size.height
    });
  },

  /**
   * Handle a scripting message from outside the extension (typically sent by
   * PDFScriptingAPI in a page containing the extension) to interact with the
   * plugin.
   * @param {MessageObject} message the message to handle.
   */
  handleScriptingMessage: function(message) {
    if (this.parentWindow_ != message.source) {
      this.parentWindow_ = message.source;
      this.parentOrigin_ = message.origin;
      // Ensure that we notify the embedder if the document is loaded.
      if (this.loadState_ != LoadState.LOADING)
        this.sendDocumentLoadedMessage_();
    }

    if (this.handlePrintPreviewScriptingMessage_(message))
      return;

    // Delay scripting messages from users of the scripting API until the
    // document is loaded. This simplifies use of the APIs.
    if (this.loadState_ != LoadState.SUCCESS) {
      this.delayedScriptingMessages_.push(message);
      return;
    }

    switch (message.data.type.toString()) {
      case 'getSelectedText':
      case 'print':
      case 'selectAll':
        this.plugin_.postMessage(message.data);
        break;
    }
  },

  /**
   * @private
   * Handle scripting messages specific to print preview.
   * @param {MessageObject} message the message to handle.
   * @return {boolean} true if the message was handled, false otherwise.
   */
  handlePrintPreviewScriptingMessage_: function(message) {
    if (!this.isPrintPreview_)
      return false;

    switch (message.data.type.toString()) {
      case 'loadPreviewPage':
        this.plugin_.postMessage(message.data);
        return true;
      case 'resetPrintPreviewMode':
        this.loadState_ = LoadState.LOADING;
        if (!this.inPrintPreviewMode_) {
          this.inPrintPreviewMode_ = true;
          this.viewport_.fitToPage();
        }

        // Stash the scroll location so that it can be restored when the new
        // document is loaded.
        this.lastViewportPosition_ = this.viewport_.position;

        // TODO(raymes): Disable these properly in the plugin.
        var printButton = $('print-button');
        if (printButton)
          printButton.parentNode.removeChild(printButton);
        var saveButton = $('save-button');
        if (saveButton)
          saveButton.parentNode.removeChild(saveButton);

        this.pageIndicator_.pageLabels = message.data.pageNumbers;

        this.plugin_.postMessage({
          type: 'resetPrintPreviewMode',
          url: message.data.url,
          grayscale: message.data.grayscale,
          // If the PDF isn't modifiable we send 0 as the page count so that no
          // blank placeholder pages get appended to the PDF.
          pageCount: (message.data.modifiable ?
                      message.data.pageNumbers.length : 0)
        });
        return true;
      case 'sendKeyEvent':
        this.handleKeyEvent_(DeserializeKeyEvent(message.data.keyEvent));
        return true;
    }

    return false;
  },

  /**
   * @private
   * Send a scripting message outside the extension (typically to
   * PDFScriptingAPI in a page containing the extension).
   * @param {Object} message the message to send.
   */
  sendScriptingMessage_: function(message) {
    if (this.parentWindow_ && this.parentOrigin_) {
      var targetOrigin;
      // Only send data back to the embedder if it is from the same origin,
      // unless we're sending it to ourselves (which could happen in the case
      // of tests). We also allow documentLoaded messages through as this won't
      // leak important information.
      if (this.parentOrigin_ == window.location.origin)
        targetOrigin = this.parentOrigin_;
      else if (message.type == 'documentLoaded')
        targetOrigin = '*';
      else
        targetOrigin = this.originalUrl_;
      this.parentWindow_.postMessage(message, targetOrigin);
    }
  },

  /**
   * @type {Viewport} the viewport of the PDF viewer.
   */
  get viewport() {
    return this.viewport_;
  },

  /**
   * Each bookmark is an Object containing a:
   * - title
   * - page (optional)
   * - array of children (themselves bookmarks)
   * @type {Array} the top-level bookmarks of the PDF.
   */
  get bookmarks() {
    return this.bookmarks_;
  }
};
