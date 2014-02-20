// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/node_bindings_linux.h"

namespace atom {

NodeBindingsLinux::NodeBindingsLinux(bool is_browser)
    : NodeBindings(is_browser) {
}

NodeBindingsLinux::~NodeBindingsLinux() {
}

void NodeBindingsLinux::PollEvents() {
}

// static
NodeBindings* NodeBindings::Create(bool is_browser) {
  return new NodeBindingsLinux(is_browser);
}

}  // namespace atom
