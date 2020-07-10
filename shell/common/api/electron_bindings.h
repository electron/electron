// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_ELECTRON_BINDINGS_H_
#define SHELL_COMMON_API_ELECTRON_BINDINGS_H_

#include <list>
#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/process_metrics.h"
#include "base/strings/string16.h"
#include "shell/common/gin_helper/promise.h"
#include "uv.h"  // NOLINT(build/include_directory)

namespace gin_helper {
class Arguments;
class Dictionary;
}  // namespace gin_helper

namespace memory_instrumentation {
class GlobalMemoryDump;
}

namespace node {
class Environment;
}

namespace electron {

class ElectronBindings {
 public:
  explicit ElectronBindings(uv_loop_t* loop);
  virtual ~ElectronBindings();

  // Add process._linkedBinding function, which behaves like process.binding
  // but load native code from Electron instead.
  void BindTo(v8::Isolate* isolate, v8::Local<v8::Object> process);

  // Should be called when a node::Environment has been destroyed.
  void EnvironmentDestroyed(node::Environment* env);

  static void BindProcess(v8::Isolate* isolate,
                          gin_helper::Dictionary* process,
                          base::ProcessMetrics* metrics);

  static void Log(const base::string16& message);
  static void Crash();

 private:
  static void Hang();
  static v8::Local<v8::Value> GetHeapStatistics(v8::Isolate* isolate);
  static v8::Local<v8::Value> GetCreationTime(v8::Isolate* isolate);
  static v8::Local<v8::Value> GetSystemMemoryInfo(v8::Isolate* isolate,
                                                  gin_helper::Arguments* args);
  static v8::Local<v8::Promise> GetProcessMemoryInfo(v8::Isolate* isolate);
  static v8::Local<v8::Value> GetBlinkMemoryInfo(v8::Isolate* isolate);
  static v8::Local<v8::Value> GetCPUUsage(base::ProcessMetrics* metrics,
                                          v8::Isolate* isolate);
  static v8::Local<v8::Value> GetIOCounters(v8::Isolate* isolate);
  static bool TakeHeapSnapshot(v8::Isolate* isolate,
                               const base::FilePath& file_path);

  void ActivateUVLoop(v8::Isolate* isolate);

  static void OnCallNextTick(uv_async_t* handle);

  static void DidReceiveMemoryDump(
      v8::Global<v8::Context> context,
      gin_helper::Promise<gin_helper::Dictionary> promise,
      bool success,
      std::unique_ptr<memory_instrumentation::GlobalMemoryDump> dump);

  uv_async_t call_next_tick_async_;
  std::list<node::Environment*> pending_next_ticks_;
  std::unique_ptr<base::ProcessMetrics> metrics_;

  DISALLOW_COPY_AND_ASSIGN(ElectronBindings);
};

}  // namespace electron

#endif  // SHELL_COMMON_API_ELECTRON_BINDINGS_H_
