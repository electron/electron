// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_BINDINGS_MAC_H_
#define ATOM_COMMON_NODE_BINDINGS_MAC_H_

#include "atom/common/node_bindings.h"
#include "base/compiler_specific.h"

namespace atom {

class NodeBindingsMac : public NodeBindings {
 public:
  explicit NodeBindingsMac(bool is_browser);
  virtual ~NodeBindingsMac();

  void RunMessageLoop() override;

 private:
  // Called when uv's watcher queue changes.
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  void PollEvents() override;

  // Kqueue to poll for uv's backend fd.
  int kqueue_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsMac);
};

}  // namespace atom

#endif  // ATOM_COMMON_NODE_BINDINGS_MAC_H_
