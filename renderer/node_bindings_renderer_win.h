// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_NODE_BINDINGS_RENDERER_WIN_H_
#define ATOM_RENDERER_NODE_BINDINGS_RENDERER_WIN_H_

#include "base/compiler_specific.h"
#include "common/node_bindings.h"

namespace atom {

class NodeBindingsRendererWin : public NodeBindings {
 public:
  NodeBindingsRendererWin();
  virtual ~NodeBindingsRendererWin();

  virtual void PrepareMessageLoop() OVERRIDE;
  virtual void RunMessageLoop() OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeBindingsRendererWin);
};

}  // namespace atom

#endif  // ATOM_RENDERER_NODE_BINDINGS_RENDERER_WIN_H_
