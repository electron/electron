// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NODE_BINDINGS_BROWSER_WIN_H_
#define ATOM_BROWSER_NODE_BINDINGS_BROWSER_WIN_H_

#include "base/compiler_specific.h"
#include "common/node_bindings.h"

namespace atom {

class NodeBindingsBrowserWin : public NodeBindings {
 public:
  NodeBindingsBrowserWin();
  virtual ~NodeBindingsBrowserWin();

  virtual void PrepareMessageLoop() OVERRIDE;
  virtual void RunMessageLoop() OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeBindingsBrowserWin);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NODE_BINDINGS_BROWSER_WIN_H_
