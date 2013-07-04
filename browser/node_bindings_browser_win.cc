// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/node_bindings_browser_win.h"

namespace atom {

NodeBindingsBrowserWin::NodeBindingsBrowserWin()
    : NodeBindings(true) {
}

NodeBindingsBrowserWin::~NodeBindingsBrowserWin() {
}

void NodeBindingsBrowserWin::PrepareMessageLoop() {
}

void NodeBindingsBrowserWin::RunMessageLoop() {
}

// static
NodeBindings* NodeBindings::CreateInBrowser() {
  return new NodeBindingsBrowserWin();
}

}  // namespace atom
