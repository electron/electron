// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MICROTASKS_RUNNER_H_
#define ELECTRON_SHELL_BROWSER_MICROTASKS_RUNNER_H_

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/task/task_observer.h"

namespace v8 {
class Isolate;
}

namespace electron {

// Microtasks like promise resolution, are run at the end of the current
// task. This class implements a task observer that runs tells v8 to run them.
// Microtasks runner implementation is based on the EndOfTaskRunner in blink.
// Node follows the kExplicit MicrotasksPolicy, and we do the same in browser
// process. Hence, we need to have this task observer to flush the queued
// microtasks.
class MicrotasksRunner : public base::TaskObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) = 0;
  };

  explicit MicrotasksRunner(v8::Isolate* isolate);
  ~MicrotasksRunner() override;

  static void AddObserver(Observer* observer);
  static void RemoveObserver(Observer* observer);

  void NotifyBeforeDispose();

  // base::TaskObserver
  void WillProcessTask(const base::PendingTask& pending_task,
                       bool was_blocked_or_low_priority) override;
  void DidProcessTask(const base::PendingTask& pending_task) override;

 private:
  raw_ptr<v8::Isolate> isolate_;
  base::ObserverList<Observer> observers_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MICROTASKS_RUNNER_H_
