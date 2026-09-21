// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_event_emitter.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "gin/converter.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

v8::Global<v8::Object>* GetEventEmitterPrototypeReference() {
  static base::NoDestructor<v8::Global<v8::Object>> event_emitter_prototype;
  return event_emitter_prototype.get();
}

// EventEmitter.prototype.emit as it was when the prototype was registered,
// which is before any app code had a chance to replace it.
v8::Global<v8::Value>* GetOriginalEmitReference() {
  static base::NoDestructor<v8::Global<v8::Value>> original_emit;
  return original_emit.get();
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
  static base::NoDestructor<v8::Eternal<v8::String>> value_key(
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
               ->HasOwnProperty(context, value_key->Get(isolate))
               .FromMaybe(false) ||
          !descriptor.As<v8::Object>()
               ->Get(context, value_key->Get(isolate))
               .ToLocal(value)) {
        return DataProperty::kUnknown;
      }
      return DataProperty::kValue;
    }
    current = holder->GetPrototype();
  }
  return DataProperty::kUnknown;
}

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
  if (prototype
          ->Get(isolate->GetCurrentContext(),
                gin::StringToSymbol(isolate, "emit"))
          .ToLocal(&emit) &&
      emit->IsFunction()) {
    GetOriginalEmitReference()->Reset(isolate, emit);
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

bool MayHaveEventListeners(v8::Isolate* isolate,
                           v8::Local<v8::Object> emitter,
                           std::string_view name) {
  // emit('error') throws when nothing handles it, so it is never a no-op.
  if (name == "error")
    return true;

  v8::Global<v8::Value>* original_emit = GetOriginalEmitReference();
  v8::Local<v8::Context> context;
  if (original_emit->IsEmpty() ||
      !emitter->GetCreationContext(isolate).ToLocal(&context)) {
    return true;
  }

  static base::NoDestructor<v8::Eternal<v8::String>> emit_key(
      isolate, gin::StringToSymbol(isolate, "emit"));
  static base::NoDestructor<v8::Eternal<v8::String>> events_key(
      isolate, gin::StringToSymbol(isolate, "_events"));

  // Nothing below runs JavaScript, so asking has no effect of its own; when
  // only JavaScript could answer, the answer is "yes" and emit() decides.

  // The same lookup emit() does: with no `_events`, or no entry for |name| in
  // it, emit() returns false without calling anything. An event that does have
  // listeners is settled here, by the emitter's own `_events`.
  v8::Local<v8::Value> events;
  if (ReadDataProperty(isolate, context, emitter, events_key->Get(isolate),
                       &events) == DataProperty::kUnknown) {
    return true;
  }
  if (!events.IsEmpty() && !events->IsUndefined()) {
    if (!events->IsObject())
      return true;
    v8::Local<v8::Value> listeners;
    const DataProperty found =
        ReadDataProperty(isolate, context, events.As<v8::Object>(),
                         gin::StringToSymbol(isolate, name), &listeners);
    if (found == DataProperty::kUnknown ||
        (found == DataProperty::kValue && !listeners->IsUndefined())) {
      return true;
    }
  }

  // Nothing listens. Anything other than Node's own emit() - replaced on the
  // instance, on a subclass or on EventEmitter.prototype itself - still sees
  // every event, so only the original can be skipped.
  v8::Local<v8::Value> emit;
  return ReadDataProperty(isolate, context, emitter, emit_key->Get(isolate),
                          &emit) != DataProperty::kValue ||
         !emit->StrictEquals(original_emit->Get(isolate));
}

}  // namespace electron

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_event_emitter, Initialize)
