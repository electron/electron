// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

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

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsMac);
};

}  // namespace atom
