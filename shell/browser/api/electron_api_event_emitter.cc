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

  // These are plain data properties unless somebody made them accessors;
  // whatever such an accessor throws is answered with "yes".
  v8::TryCatch try_catch(isolate);

  // Anything other than Node's own emit() - replaced on the instance, on a
  // subclass or on EventEmitter.prototype itself - sees every event whether
  // or not it has listeners, so only the original can be skipped.
  v8::Local<v8::Value> emit;
  if (!emitter->Get(context, emit_key->Get(isolate)).ToLocal(&emit) ||
      !emit->StrictEquals(original_emit->Get(isolate))) {
    return true;
  }

  // The same lookup emit() does: no `_events`, or no entry for |name| in it,
  // and it returns false without calling anything.
  v8::Local<v8::Value> events;
  if (!emitter->Get(context, events_key->Get(isolate)).ToLocal(&events))
    return true;
  if (events->IsUndefined())
    return false;
  if (!events->IsObject())
    return true;

  v8::Local<v8::Value> listeners;
  if (!events.As<v8::Object>()
           ->Get(context, gin::StringToSymbol(isolate, name))
           .ToLocal(&listeners)) {
    return true;
  }
  return !listeners->IsUndefined();
}

}  // namespace electron

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_event_emitter, Initialize)
