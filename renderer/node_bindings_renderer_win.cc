// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/node_bindings_renderer_win.h"

namespace atom {

NodeBindingsRendererWin::NodeBindingsRendererWin()
    : NodeBindings(false) {
}

NodeBindingsRendererWin::~NodeBindingsRendererWin() {
}

void NodeBindingsRendererWin::PrepareMessageLoop() {
}

void NodeBindingsRendererWin::RunMessageLoop() {
}

// static
NodeBindings* NodeBindings::CreateInRenderer() {
  return new NodeBindingsRendererWin();
}

}  // namespace atom
