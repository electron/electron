// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MICROTASKS_RUNNER_H_
#define ELECTRON_SHELL_BROWSER_MICROTASKS_RUNNER_H_

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/task/task_observer.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/destroyable.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/cppgc/type-traits.h"
#include "v8/include/v8-callbacks.h"

namespace v8 {
class Isolate;
}

namespace electron {

// Microtasks like promise resolution, are run at the end of the current
// task. This class implements a task observer that tells v8 to run them.
// Microtasks runner implementation is based on the EndOfTaskRunner in blink.
// Node follows the kExplicit MicrotasksPolicy, and we do the same in browser
// process. Hence, we need to have this task observer to flush the queued
// microtasks.
class MicrotasksRunner : public base::TaskObserver {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void OnBeforeMicrotasksRunnerDispose() = 0;

   private:
    friend class MicrotasksRunner;
    virtual bool IsAlive() const;
  };

  template <typename T>
  class WrappableObserver final : public Observer {
   public:
    explicit WrappableObserver(T* observer) : observer_(observer) {}

    void OnBeforeMicrotasksRunnerDispose() override;

   private:
    bool IsAlive() const override;
    cppgc::WeakPersistent<T> observer_;
  };

  explicit MicrotasksRunner(v8::Isolate* isolate);
  ~MicrotasksRunner() override;

  // Registers a native owned observer. The owner must call RemoveObserver()
  // before destroying it. NotifyBeforeDispose() pops observers before invoking
  // them, so callbacks do not need to remove themselves.
  template <typename T>
  static void AddObserver(T* observer) {
    static_assert(
        std::derived_from<T, Observer>,
        "Native observers must derive from MicrotasksRunner::Observer");
    static_assert(!cppgc::IsGarbageCollectedOrMixinTypeV<T>,
                  "cppgc managed observers must use AddWrappableObserver()");
    AddNativeObserver(observer);
  }

  // Registers a cppgc managed gin wrapper through a runner owned adapter. The
  // adapter holds a WeakPersistent, is pruned after collection, and temporarily
  // roots the wrapper while invalidating its JavaScript peer and running native
  // shutdown. Wrappers must not call RemoveObserver().
  template <typename T>
  static void AddWrappableObserver(T* observer) {
    static_assert(cppgc::IsGarbageCollectedTypeV<T>,
                  "AddWrappableObserver() requires a cppgc managed type");
    static_assert(std::derived_from<T, gin::WrappableBase>,
                  "AddWrappableObserver() requires a gin::Wrappable");
    static_assert(
        requires(T* value) {
          { value->OnBeforeMicrotasksRunnerDispose() } -> std::same_as<void>;
        },
        "Wrappable observers must provide "
        "void OnBeforeMicrotasksRunnerDispose()");
    AddOwnedObserver(std::make_unique<WrappableObserver<T>>(observer));
  }
  static void RemoveObserver(Observer* observer);

  void NotifyBeforeDispose();

  // base::TaskObserver
  void WillProcessTask(const base::PendingTask& pending_task,
                       bool was_blocked_or_low_priority) override;
  void DidProcessTask(const base::PendingTask& pending_task) override;

 private:
  struct Observation {
    explicit Observation(Observer* observer_in);
    explicit Observation(std::unique_ptr<Observer> owned_in);
    Observation(Observation&&);
    Observation& operator=(Observation&&);
    ~Observation();

    raw_ptr<Observer> observer;
    std::unique_ptr<Observer> owned;
  };

  static void AddNativeObserver(Observer* observer);
  static void AddOwnedObserver(std::unique_ptr<Observer> observer);
  static void OnGarbageCollection(v8::Isolate* isolate,
                                  v8::GCType type,
                                  v8::GCCallbackFlags flags,
                                  void* data);
  static v8::Isolate* GetIsolate();
  void PruneDeadWrappableObservers();

  raw_ptr<v8::Isolate> isolate_;
  std::vector<Observation> observers_;
};

template <typename T>
bool MicrotasksRunner::WrappableObserver<T>::IsAlive() const {
  return observer_.Get();
}

template <typename T>
void MicrotasksRunner::WrappableObserver<T>::OnBeforeMicrotasksRunnerDispose() {
  cppgc::Persistent<T> observer(observer_.Get());
  if (!observer)
    return;

  gin_helper::Destroyable::MarkDestroyed(GetIsolate(), observer.Get());
  observer->OnBeforeMicrotasksRunnerDispose();
}

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MICROTASKS_RUNNER_H_
