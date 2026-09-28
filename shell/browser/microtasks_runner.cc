
// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/microtasks_runner.h"

#include <algorithm>

#include "base/check_op.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace electron {

namespace {

MicrotasksRunner* g_microtasks_runner = nullptr;

}  // namespace

bool MicrotasksRunner::Observer::IsAlive() const {
  return true;
}

MicrotasksRunner::Observation::Observation(Observer* observer_in)
    : native(observer_in) {}

MicrotasksRunner::Observation::Observation(std::unique_ptr<Observer> owned_in)
    : owned(std::move(owned_in)) {}

MicrotasksRunner::Observation::Observation(Observation&&) = default;

MicrotasksRunner::Observation& MicrotasksRunner::Observation::operator=(
    Observation&&) = default;

MicrotasksRunner::Observation::~Observation() = default;

MicrotasksRunner::MicrotasksRunner(v8::Isolate* isolate) : isolate_(isolate) {
  CHECK(!g_microtasks_runner);
  g_microtasks_runner = this;
  isolate_->AddGCEpilogueCallback(OnGarbageCollection, this);
}

MicrotasksRunner::~MicrotasksRunner() {
  CHECK_EQ(g_microtasks_runner, this);
  CHECK(observers_.empty());
  isolate_->RemoveGCEpilogueCallback(OnGarbageCollection, this);
  g_microtasks_runner = nullptr;
}

// static
void MicrotasksRunner::AddNativeObserver(Observer* observer) {
  CHECK(g_microtasks_runner);
  CHECK(std::ranges::none_of(g_microtasks_runner->observers_,
                             [observer](const Observation& item) {
                               return item.observer() == observer;
                             }));
  g_microtasks_runner->observers_.emplace_back(observer);
}

// static
void MicrotasksRunner::AddOwnedObserver(std::unique_ptr<Observer> observer) {
  CHECK(g_microtasks_runner);
  g_microtasks_runner->observers_.emplace_back(std::move(observer));
}

// static
void MicrotasksRunner::OnGarbageCollection(v8::Isolate*,
                                           v8::GCType,
                                           v8::GCCallbackFlags,
                                           void* data) {
  static_cast<MicrotasksRunner*>(data)->PruneDeadWrappableObservers();
}

// static
v8::Isolate* MicrotasksRunner::GetIsolate() {
  CHECK(g_microtasks_runner);
  return g_microtasks_runner->isolate_;
}

void MicrotasksRunner::PruneDeadWrappableObservers() {
  std::erase_if(observers_, [](const Observation& item) {
    return item.owned && !item.observer()->IsAlive();
  });
}

// static
void MicrotasksRunner::RemoveObserver(Observer* observer) {
  if (g_microtasks_runner) {
    std::erase_if(g_microtasks_runner->observers_,
                  [observer](const Observation& item) {
                    return item.observer() == observer;
                  });
  }
}

void MicrotasksRunner::NotifyBeforeDispose() {
  // Native resources are normally registered after their dependencies.
  // Unwind them in reverse registration order.
  // Pop before invoking so callbacks may safely remove other observers or
  // register new work, which becomes the next entry processed.
  while (!observers_.empty()) {
    Observation observation = std::move(observers_.back());
    observers_.pop_back();
    Observer* observer = observation.observer();
    observer->OnBeforeMicrotasksRunnerDispose();
  }
}

void MicrotasksRunner::WillProcessTask(const base::PendingTask& pending_task,
                                       bool was_blocked_or_low_priority) {}

void MicrotasksRunner::DidProcessTask(const base::PendingTask& pending_task) {
  // In the browser process we follow Node.js microtask policy of kExplicit
  // and let the MicrotaskRunner which is a task observer for chromium UI thread
  // scheduler run the microtask checkpoint. This worked fine because Node.js
  // also runs microtasks through its task queue, but after
  // https://github.com/electron/electron/issues/20013 Node.js now performs its
  // own microtask checkpoint and it may happen is some situations that there is
  // contention for performing checkpoint between Node.js and chromium, ending
  // up Node.js delaying its callbacks. To fix this, now we always lets Node.js
  // handle the checkpoint in the browser process.
  {
    v8::HandleScope handle_scope(isolate_);
    node::CallbackScope microtasks_scope(isolate_, v8::Object::New(isolate_),
                                         {0, 0});
  }
}

}  // namespace electron
