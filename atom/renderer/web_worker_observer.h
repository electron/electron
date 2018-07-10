// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_WEB_WORKER_OBSERVER_H_
#define ATOM_RENDERER_WEB_WORKER_OBSERVER_H_

#include "base/macros.h"
#include "v8/include/v8.h"

namespace atom {

class AtomBindings;
class NodeBindings;

// Watches for WebWorker and insert node integration to it.
class WebWorkerObserver {
 public:
  // Returns the WebWorkerObserver for current worker thread.
  static WebWorkerObserver* GetCurrent();

  void ContextCreated(v8::Local<v8::Context> context);
  void ContextWillDestroy(v8::Local<v8::Context> context);

 private:
  WebWorkerObserver();
  ~WebWorkerObserver();

  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<AtomBindings> atom_bindings_;

  DISALLOW_COPY_AND_ASSIGN(WebWorkerObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_WEB_WORKER_OBSERVER_H_
