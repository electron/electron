// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_

#include <string>
#include <string_view>

#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace v8 {
template <typename T>
class Local;
class Context;
class Object;
class Isolate;
}  // namespace v8

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate);

// Gives |prototype| - the one every native emitter of |context| inherits its
// EventEmitter methods through - native on() / off() / removeAllListeners()
// that keep each emitter's EventListenerSet up to date.
void InstallListenerTracking(v8::Local<v8::Context> context,
                             v8::Local<v8::Object> prototype);

// The names of the events that JavaScript listens for on one native emitter,
// kept in native memory so that deciding whether an event is worth emitting
// costs a hash lookup and no call into V8.
//
// InstallListenerTracking() keeps it up to date: every native emitter
// inherits on() / off() / removeAllListeners() from the prototype it sets up,
// and those are native and report each change here. Until the first emit has
// tied the set to its wrapper with Link() nothing is known, and every event
// may be observed.
class EventListenerSet {
 public:
  EventListenerSet();
  ~EventListenerSet();

  // disable copy
  EventListenerSet(const EventListenerSet&) = delete;
  EventListenerSet& operator=(const EventListenerSet&) = delete;

  // Whether emitting |name| could reach any JavaScript. An unhandled 'error'
  // throws, so that one always can.
  bool MayObserve(std::string_view name) const {
    return !linked_ || observe_all_ || names_.contains(name) || name == "error";
  }

  bool linked() const { return linked_; }

  // Makes the set reachable from |wrapper| and fills it with the listeners
  // that |wrapper| already has. Runs no JavaScript: what only JavaScript could
  // answer - a proxy or an accessor in the way, an emit() that is not the one
  // native emitters inherit - leaves every event observable.
  void Link(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  // Makes the set unreachable from |wrapper|. For emitters whose wrapper can
  // outlive them.
  void Unlink(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  void SetObserved(std::string_view name, bool observed);
  void Clear() { names_.clear(); }

 private:
  bool linked_ = false;
  bool observe_all_ = false;
  absl::flat_hash_set<std::string> names_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_EVENT_EMITTER_H_
