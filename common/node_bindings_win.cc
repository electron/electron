// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/node_bindings_win.h"

#include <windows.h>

#include "base/logging.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

NodeBindingsWin::NodeBindingsWin(bool is_browser)
    : NodeBindings(is_browser) {
}

NodeBindingsWin::~NodeBindingsWin() {
}

void NodeBindingsWin::PollEvents() {
  Sleep(50);
}

// static
NodeBindings* NodeBindings::Create(bool is_browser) {
  return new NodeBindingsWin(is_browser);
}

}  // namespace atom
