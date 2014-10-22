// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_VIEW_WEB_VIEW_GUEST_H_
#define ATOM_BROWSER_WEB_VIEW_WEB_VIEW_GUEST_H_

namespace atom {

// A WebViewGuest provides the browser-side implementation of the <webview> API
// and manages the dispatch of <webview> extension events. WebViewGuest is
// created on attachment. That is, when a guest WebContents is associated with
// a particular embedder WebContents. This happens on either initial navigation
// or through the use of the New Window API, when a new window is attached to
// a particular <webview>.

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_VIEW_WEB_VIEW_GUEST_H_
