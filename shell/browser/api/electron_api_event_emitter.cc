// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_event_emitter.h"

#include <string>
#include <tuple>
#include <utility>

#include "base/check.h"
#include "base/no_destructor.h"
#include "gin/converter.h"
#include "shell/common/gin_helper/node_event_emitter.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

v8::Global<v8::Object>* GetEventEmitterPrototypeReference() {
  static base::NoDestructor<v8::Global<v8::Object>> event_emitter_prototype;
  return event_emitter_prototype.get();
}

// Node's EventEmitter.prototype.emit as it was when the prototype was
// registered, which is before any app code had a chance to replace it.
v8::Global<v8::Value>* GetOriginalEmitReference() {
  static base::NoDestructor<v8::Global<v8::Value>> original_emit;
  return original_emit.get();
}

v8::Local<v8::String> EmitKey(v8::Isolate* isolate) {
  static const v8::Eternal<v8::String> key(
      isolate, gin::StringToSymbol(isolate, "emit"));
  return key.Get(isolate);
}

// The private slot of a wrapper that holds its EventListenerSet.
v8::Local<v8::Private> ListenerSetKey(v8::Isolate* isolate) {
  return v8::Private::ForApi(
      isolate, gin::StringToSymbol(isolate, "electron.eventListenerSet"));
}

// The outcome of reading a property without running any JavaScript.
enum class DataProperty {
  kAbsent,   // not on the object or anything it inherits from
  kValue,    // a data property; its value was read
  kUnknown,  // only JavaScript could tell: an accessor or a proxy is involved
};

// Reads |key| from |object| or its prototype chain the way a property load
// would, except that nothing is invoked to do so: a proxy or an accessor on
// the way makes the answer kUnknown rather than running its trap or getter.
DataProperty ReadDataProperty(v8::Isolate* isolate,
                              v8::Local<v8::Context> context,
                              v8::Local<v8::Object> object,
                              v8::Local<v8::String> key,
                              v8::Local<v8::Value>* value) {
  static const v8::Eternal<v8::String> value_key(
      isolate, gin::StringToSymbol(isolate, "value"));

  // Far longer than the chain of any native emitter.
  constexpr int kMaxChainLength = 16;

  v8::Local<v8::Value> current = object;
  for (int i = 0; i < kMaxChainLength; i++) {
    if (current->IsNull())
      return DataProperty::kAbsent;
    if (!current->IsObject() || current->IsProxy())
      return DataProperty::kUnknown;

    v8::Local<v8::Object> holder = current.As<v8::Object>();
    bool has_own = false;
    if (!holder->HasOwnProperty(context, key).To(&has_own))
      return DataProperty::kUnknown;
    if (has_own) {
      // An accessor's descriptor has get/set where a data property's has
      // value. The descriptor is a fresh plain object, so reading its own
      // |value| runs nothing either.
      v8::Local<v8::Value> descriptor;
      if (!holder->GetOwnPropertyDescriptor(context, key)
               .ToLocal(&descriptor) ||
          !descriptor->IsObject() ||
          !descriptor.As<v8::Object>()
               ->HasOwnProperty(context, value_key.Get(isolate))
               .FromMaybe(false) ||
          !descriptor.As<v8::Object>()
               ->Get(context, value_key.Get(isolate))
               .ToLocal(value)) {
        return DataProperty::kUnknown;
      }
      return DataProperty::kValue;
    }
    current = holder->GetPrototype();
  }
  return DataProperty::kUnknown;
}

// Whether |wrapper| gets its emit() from Node's EventEmitter.prototype, with
// nothing of its own in front of it, and that emit() is still the original.
// Anything else - an emit() assigned to the wrapper or to a subclass, a
// patched EventEmitter.prototype.emit - sees every event whether or not it
// has listeners.
bool InheritsOriginalEmit(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Object> wrapper) {
  if (GetEventEmitterPrototypeReference()->IsEmpty() ||
      GetOriginalEmitReference()->IsEmpty()) {
    return false;
  }
  v8::Local<v8::Object> registered =
      GetEventEmitterPrototypeReference()->Get(isolate);

  constexpr int kMaxChainLength = 16;
  v8::Local<v8::Value> current = wrapper;
  for (int i = 0; i < kMaxChainLength; i++) {
    if (!current->IsObject() || current->IsProxy())
      return false;
    v8::Local<v8::Object> holder = current.As<v8::Object>();
    if (holder->StrictEquals(registered)) {
      v8::Local<v8::Value> emit;
      return ReadDataProperty(isolate, context, holder, EmitKey(isolate),
                              &emit) == DataProperty::kValue &&
             emit->StrictEquals(GetOriginalEmitReference()->Get(isolate));
    }
    bool has_own = true;
    if (!holder->HasOwnProperty(context, EmitKey(isolate)).To(&has_own) ||
        has_own) {
      return false;
    }
    current = holder->GetPrototype();
  }
  return false;
}

// The set that EventListenerSet::Link() tied to |emitter|, if any.
electron::EventListenerSet* ListenerSetOf(v8::Isolate* isolate,
                                          v8::Local<v8::Value> emitter) {
  if (!emitter->IsObject())
    return nullptr;
  v8::Local<v8::Object> object = emitter.As<v8::Object>();
  v8::Local<v8::Context> context;
  v8::Local<v8::Value> slot;
  if (!object->GetCreationContext(isolate).ToLocal(&context) ||
      !object->GetPrivate(context, ListenerSetKey(isolate)).ToLocal(&slot) ||
      !slot->IsExternal()) {
    return nullptr;
  }
  return static_cast<electron::EventListenerSet*>(
      slot.As<v8::External>()->Value(v8::kExternalPointerTypeTagDefault));
}

// setEventEmitterPrototype(prototype): Node's EventEmitter.prototype, which
// the prototype set up by InstallListenerTracking() inherits from.
void SetEventEmitterPrototype(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info[0]->IsObject()) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8::String::NewFromUtf8Literal(
            isolate, "Expected an EventEmitter prototype")));
    return;
  }
  v8::Local<v8::Object> prototype = info[0].As<v8::Object>();
  GetEventEmitterPrototypeReference()->Reset(isolate, prototype);

  v8::Local<v8::Value> emit;
  if (ReadDataProperty(isolate, isolate->GetCurrentContext(), prototype,
                       EmitKey(isolate), &emit) == DataProperty::kValue &&
      emit->IsFunction()) {
    GetOriginalEmitReference()->Reset(isolate, emit);
  }
}

// gin_helper::ListenerChangeCallback for the prototype set up by
// InstallListenerTracking(): the emitter's set follows its listener table.
void OnListenerChange(v8::Isolate* isolate,
                      v8::Local<v8::Object> emitter,
                      v8::Local<v8::Value> type,
                      gin_helper::ListenerChange change) {
  electron::EventListenerSet* listeners = ListenerSetOf(isolate, emitter);
  if (!listeners)
    return;
  if (change == gin_helper::ListenerChange::kUnobservedAll) {
    listeners->Clear();
    return;
  }
  // Native code only emits events with string names.
  std::string name;
  if (type->IsString() && gin::ConvertFromV8(isolate, type, &name)) {
    listeners->SetObserved(name,
                           change == gin_helper::ListenerChange::kObserved);
  }
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  NODE_SET_METHOD(exports, "setEventEmitterPrototype",
                  &SetEventEmitterPrototype);
}

}  // namespace

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate) {
  CHECK(!GetEventEmitterPrototypeReference()->IsEmpty());
  return GetEventEmitterPrototypeReference()->Get(isolate);
}

void InstallListenerTracking(v8::Local<v8::Context> context,
                             v8::Local<v8::Object> prototype) {
  gin_helper::InstallListenerMethods(context, prototype, &OnListenerChange);
}

EventListenerSet::EventListenerSet() = default;
EventListenerSet::~EventListenerSet() = default;

void EventListenerSet::Link(v8::Isolate* isolate,
                            v8::Local<v8::Object> wrapper) {
  if (linked_)
    return;

  static const v8::Eternal<v8::String> events_key(
      isolate, gin::StringToSymbol(isolate, "_events"));

  v8::Local<v8::Context> context;
  if (!wrapper->GetCreationContext(isolate).ToLocal(&context))
    return;

  // From here on() and friends can reach the set. Listeners added before now
  // are picked up below; nothing in between runs JavaScript.
  if (!wrapper
           ->SetPrivate(context, ListenerSetKey(isolate),
                        v8::External::New(isolate, this,
                                          v8::kExternalPointerTypeTagDefault))
           .FromMaybe(false)) {
    return;
  }
  linked_ = true;

  if (!InheritsOriginalEmit(isolate, context, wrapper)) {
    observe_all_ = true;
    return;
  }

  v8::Local<v8::Value> events;
  if (ReadDataProperty(isolate, context, wrapper, events_key.Get(isolate),
                       &events) == DataProperty::kUnknown) {
    observe_all_ = true;
    return;
  }
  if (events.IsEmpty() || events->IsUndefined())
    return;
  v8::Local<v8::Array> names;
  if (!events->IsObject() || events->IsProxy() ||
      !events.As<v8::Object>()->GetOwnPropertyNames(context).ToLocal(&names)) {
    observe_all_ = true;
    return;
  }
  for (uint32_t i = 0; i < names->Length(); i++) {
    v8::Local<v8::Value> name;
    std::string utf8;
    if (!names->Get(context, i).ToLocal(&name) || !name->IsString() ||
        !gin::ConvertFromV8(isolate, name, &utf8)) {
      observe_all_ = true;
      return;
    }
    names_.insert(std::move(utf8));
  }
}

void EventListenerSet::Unlink(v8::Isolate* isolate,
                              v8::Local<v8::Object> wrapper) {
  if (!linked_)
    return;
  v8::Local<v8::Context> context;
  if (wrapper->GetCreationContext(isolate).ToLocal(&context)) {
    std::ignore = wrapper->SetPrivate(context, ListenerSetKey(isolate),
                                      v8::Undefined(isolate));
  }
}

void EventListenerSet::SetObserved(std::string_view name, bool observed) {
  if (observed) {
    names_.emplace(name);
  } else if (auto it = names_.find(name); it != names_.end()) {
    names_.erase(it);
  }
}

}  // namespace electron

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_event_emitter, Initialize)
