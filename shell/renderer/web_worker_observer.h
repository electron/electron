// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_WEB_WORKER_OBSERVER_H_
#define ELECTRON_SHELL_RENDERER_WEB_WORKER_OBSERVER_H_

#include <memory>
#include <set>

#include "v8/include/v8.h"

namespace node {

class Environment;

}  // namespace node

namespace electron {

class ElectronBindings;
class NodeBindings;

// Watches for WebWorker and insert node integration to it.
class WebWorkerObserver {
 public:
  WebWorkerObserver();
  ~WebWorkerObserver();

  // Returns the WebWorkerObserver for current worker thread.
  static WebWorkerObserver* GetCurrent();
  // Creates a new WebWorkerObserver for a given context.
  static WebWorkerObserver* Create();

  // disable copy
  WebWorkerObserver(const WebWorkerObserver&) = delete;
  WebWorkerObserver& operator=(const WebWorkerObserver&) = delete;

  void WorkerScriptReadyForEvaluation(v8::Local<v8::Context> context);
  void ContextWillDestroy(v8::Local<v8::Context> context);

 private:
  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<ElectronBindings> electron_bindings_;
  std::set<std::shared_ptr<node::Environment>> environments_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_WEB_WORKER_OBSERVER_H_
