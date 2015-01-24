// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_BINDINGS_LINUX_H_
#define ATOM_COMMON_NODE_BINDINGS_LINUX_H_

#include "base/compiler_specific.h"
#include "atom/common/node_bindings.h"

namespace atom {

class NodeBindingsLinux : public NodeBindings {
 public:
  explicit NodeBindingsLinux(bool is_browser);
  virtual ~NodeBindingsLinux();

  void RunMessageLoop() override;

 private:
  // Called when uv's watcher queue changes.
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  void PollEvents() override;

  // Epoll to poll for uv's backend fd.
  int epoll_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsLinux);
};

}  // namespace atom

#endif  // ATOM_COMMON_NODE_BINDINGS_LINUX_H_
