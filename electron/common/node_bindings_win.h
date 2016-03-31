// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_COMMON_NODE_BINDINGS_WIN_H_
#define ELECTRON_COMMON_NODE_BINDINGS_WIN_H_

#include "electron/common/node_bindings.h"
#include "base/compiler_specific.h"

namespace atom {

class NodeBindingsWin : public NodeBindings {
 public:
  explicit NodeBindingsWin(bool is_browser);
  virtual ~NodeBindingsWin();

 private:
  void PollEvents() override;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsWin);
};

}  // namespace atom

#endif  // ELECTRON_COMMON_NODE_BINDINGS_WIN_H_
