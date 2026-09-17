// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/node_event_emitter.h"

#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/stack_allocated.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "gin/converter.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-script.h"
#include "v8/include/v8-template.h"

// A C++ port of the `EventEmitter` class from Node.js's lib/events.js,
// limited to the instance API that the `events` package previously provided
// to sandboxed preloads. Function names follow lib/events.js so the two can be
// read side by side.
//
// Instances created through the constructor (including subclass instances)
// keep `_events`, `_eventsCount` and `_maxListeners` in internal fields and
// expose them to script as native data properties, so the C++ side reads them
// without a property lookup. The methods still work on any other receiver
// (`EventEmitter.call(obj)`, `EventEmitter.prototype.on.call(obj, ...)`) by
// falling back to ordinary properties, as lib/events.js's methods do.
//
// Where lib/events.js calls back into a public method (`this.emit(...)`,
// `this.on(...)`, `this.removeListener(...)`) so that subclass overrides are
// honoured, this does the same, but dispatches straight to the C++ body when
// the method has not been overridden.
//
// Every V8 call that can run script is checked; on failure (a pending
// exception or termination) the function returns without touching V8 again so
// the exception propagates to the caller unchanged.

namespace gin_helper {

namespace {

// -- Per-context state -------------------------------------------------------

// Internal field slots of the object passed as callback data to every
// function created here.
enum Slot {
  kMagic = 0,            // Smi marking this object as emitter state
  kDefaultMaxListeners,  // EventEmitter.defaultMaxListeners
  kErrorConstructor,     // the realm's Error, for `instanceof Error`
  kOnceStateTemplate,    // ObjectTemplate for once() wrapper state
  kOnceWrapper,          // the shared onceWrapper function
  kFunctionBind,         // Function.prototype.bind
  kDispatch,             // JS loop that calls an array of listeners
  // Our own prototype methods, to detect that they are not overridden.
  kFnEmit,
  kFnAddListener,
  kFnPrependListener,
  kFnRemoveListener,
  kFnRemoveAllListeners,
  // Pre-internalised property keys.
  kKeyEvents,
  kKeyEventsCount,
  kKeyMaxListeners,
  kKeyListener,
  kKeyError,
  kKeyNewListener,
  kKeyRemoveListener,
  kKeyWarned,
  kKeyLength,
  kKeyEmit,
  kKeyOn,
  kKeyPrependListener,
  kKeyRemoveAllListeners,
  kSlotCount,
};

// Internal fields of an EventEmitter instance. kTag holds the callback data
// object so instances can be told apart from other API objects that happen to
// have internal fields.
enum InstanceField {
  kTag = 0,
  kEvents,
  kEventsCount,
  kMaxListeners,
  kInstanceFieldCount,
};

// Internal fields of a once() wrapper's bound state.
enum OnceField {
  kOnceTarget = 0,
  kOnceType,
  kOnceListener,
  kOnceWrapFn,
  kOnceFired,
  kOnceFieldCount,
};

constexpr int32_t kMagicValue = 0x0EE517A7;

// Whether `value` is the callback data object of some context's EventEmitter,
// i.e. what an instance's kTag field points at.
bool IsStateData(v8::Local<v8::Value> value) {
  if (!value->IsObject())
    return false;
  v8::Local<v8::Object> object = value.As<v8::Object>();
  if (object->InternalFieldCount() != kSlotCount)
    return false;
  v8::Local<v8::Value> magic = object->GetInternalField(kMagic).As<v8::Value>();
  return magic->IsInt32() && magic.As<v8::Int32>()->Value() == kMagicValue;
}

class State {
  STACK_ALLOCATED();

 public:
  State(v8::Isolate* isolate, v8::Local<v8::Value> data)
      : isolate_(isolate),
        context_(isolate->GetCurrentContext()),
        data_(data.As<v8::Object>()) {}
  explicit State(const v8::FunctionCallbackInfo<v8::Value>& info)
      : State(info.GetIsolate(), info.Data()) {}

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const { return context_; }
  v8::Local<v8::Object> data() const { return data_; }

  v8::Local<v8::Value> Get(Slot slot) const {
    return data_->GetInternalField(slot).As<v8::Value>();
  }
  v8::Local<v8::String> Key(Slot slot) const {
    return Get(slot).As<v8::String>();
  }
  void Set(Slot slot, v8::Local<v8::Value> value) const {
    data_->SetInternalField(slot, value);
  }
  v8::Local<v8::ObjectTemplate> OnceStateTemplate() const {
    return data_->GetInternalField(kOnceStateTemplate).As<v8::ObjectTemplate>();
  }

  // Whether `object` is an instance created by this context's constructor.
  bool IsInstance(v8::Local<v8::Object> object) const {
    return object->InternalFieldCount() == kInstanceFieldCount &&
           object->GetInternalField(kTag).As<v8::Value>()->StrictEquals(data_);
  }

 private:
  v8::Isolate* isolate_;
  v8::Local<v8::Context> context_;
  v8::Local<v8::Object> data_;
};

// -- Small helpers -----------------------------------------------------------

v8::Local<v8::String> Intern(v8::Isolate* isolate, std::string_view s) {
  return gin::StringToSymbol(isolate, s);
}

v8::Local<v8::Object> NewNullProtoObject(v8::Isolate* isolate) {
  return v8::Object::New(isolate, v8::Null(isolate), nullptr, nullptr, 0);
}

[[nodiscard]] bool GetProp(const State& s,
                           v8::Local<v8::Value> object,
                           v8::Local<v8::Value> key,
                           v8::Local<v8::Value>* out) {
  return object.As<v8::Object>()->Get(s.context(), key).ToLocal(out);
}

[[nodiscard]] bool SetProp(const State& s,
                           v8::Local<v8::Value> object,
                           v8::Local<v8::Value> key,
                           v8::Local<v8::Value> value) {
  return object.As<v8::Object>()->Set(s.context(), key, value).IsJust();
}

double NumberOr(v8::Local<v8::Value> value, double fallback) {
  return value->IsNumber() ? value.As<v8::Number>()->Value() : fallback;
}

void ThrowTypeError(v8::Isolate* isolate, std::string_view message) {
  isolate->ThrowException(
      v8::Exception::TypeError(gin::StringToV8(isolate, message)));
}

void ThrowRangeError(v8::Isolate* isolate, std::string_view message) {
  isolate->ThrowException(
      v8::Exception::RangeError(gin::StringToV8(isolate, message)));
}

// `String(value)` for building messages, without running user code: objects
// are described by type only.
std::string Describe(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  if (value->IsString() || value->IsNumber() || value->IsBoolean() ||
      value->IsNullOrUndefined()) {
    v8::Local<v8::String> str;
    if (value->ToString(isolate->GetCurrentContext()).ToLocal(&str))
      return gin::V8ToString(isolate, str);
    return {};
  }
  if (value->IsSymbol()) {
    v8::Local<v8::Value> desc = value.As<v8::Symbol>()->Description(isolate);
    return base::StrCat(
        {"Symbol(",
         desc->IsString() ? gin::V8ToString(isolate, desc) : std::string(),
         ")"});
  }
  return base::StrCat(
      {"[", gin::V8ToString(isolate, value->TypeOf(isolate)), "]"});
}

// checkListener(listener)
[[nodiscard]] bool CheckListener(const State& s,
                                 v8::Local<v8::Value> listener) {
  if (listener->IsFunction())
    return true;
  ThrowTypeError(
      s.isolate(),
      base::StrCat(
          {"The \"listener\" argument must be of type Function. Received type ",
           gin::V8ToString(s.isolate(), listener->TypeOf(s.isolate()))}));
  return false;
}

// `entry.listener`, or undefined when there is none (a once() wrapper keeps
// the original listener there).
[[nodiscard]] bool GetWrappedListener(const State& s,
                                      v8::Local<v8::Value> entry,
                                      v8::Local<v8::Value>* out) {
  if (!entry->IsObject()) {
    *out = v8::Undefined(s.isolate());
    return true;
  }
  return GetProp(s, entry, s.Key(kKeyListener), out);
}

// `entry.listener || entry`
[[nodiscard]] bool UnwrapListener(const State& s,
                                  v8::Local<v8::Value> entry,
                                  v8::Local<v8::Value>* out) {
  v8::Local<v8::Value> inner;
  if (!GetWrappedListener(s, entry, &inner))
    return false;
  *out = inner->IsFunction() ? inner : entry;
  return true;
}

// `entry === listener || entry.listener === listener`
[[nodiscard]] bool MatchesListener(const State& s,
                                   v8::Local<v8::Value> entry,
                                   v8::Local<v8::Value> listener,
                                   bool* matches) {
  if (entry->StrictEquals(listener)) {
    *matches = true;
    return true;
  }
  v8::Local<v8::Value> inner;
  if (!GetWrappedListener(s, entry, &inner))
    return false;
  *matches = inner->StrictEquals(listener);
  return true;
}

[[nodiscard]] bool ReadArray(const State& s,
                             v8::Local<v8::Array> array,
                             v8::LocalVector<v8::Value>* out) {
  // (v8::Array::Iterate would be cheaper, but the handles it passes to the
  // callback do not outlive it.)
  uint32_t length = array->Length();
  out->reserve(out->size() + length);
  for (uint32_t i = 0; i < length; ++i) {
    v8::Local<v8::Value> item;
    if (!array->Get(s.context(), i).ToLocal(&item))
      return false;
    out->push_back(item);
  }
  return true;
}

// -- Receiver state access ---------------------------------------------------

// The receiver of a method call and access to its `_events`, `_eventsCount`
// and `_maxListeners`: internal fields for real instances, properties for
// anything else.
class Receiver {
  STACK_ALLOCATED();

 public:
  Receiver() = default;
  Receiver(const State& s, v8::Local<v8::Object> self)
      : self_(self), fast_(s.IsInstance(self)) {}

  // The methods are strict-mode functions in lib/events.js, so calling one
  // detached (`const { on } = emitter; on('x', f)`) throws instead of touching
  // the global object. API callbacks receive the global proxy for an
  // undefined receiver, so treat that the same as a non-object receiver.
  [[nodiscard]] static bool From(
      const State& s,
      const v8::FunctionCallbackInfo<v8::Value>& info,
      const char* method,
      Receiver* out) {
    v8::Local<v8::Value> value = info.This();
    if (value->IsObject()) {
      *out = Receiver(s, value.As<v8::Object>());
      if (out->fast_ || !value->StrictEquals(s.context()->Global()))
        return true;
    }
    ThrowTypeError(
        s.isolate(),
        base::StrCat({"EventEmitter.prototype.", method,
                      " called on undefined or a non-object receiver"}));
    return false;
  }

  v8::Local<v8::Object> self() const { return self_; }

  [[nodiscard]] bool GetEvents(const State& s,
                               v8::Local<v8::Value>* out) const {
    if (fast_) {
      *out = self_->GetInternalField(kEvents).As<v8::Value>();
      return true;
    }
    return GetProp(s, self_, s.Key(kKeyEvents), out);
  }
  [[nodiscard]] bool SetEvents(const State& s,
                               v8::Local<v8::Value> value) const {
    if (fast_) {
      self_->SetInternalField(kEvents, value);
      return true;
    }
    return SetProp(s, self_, s.Key(kKeyEvents), value);
  }
  [[nodiscard]] bool GetEventsCount(const State& s, double* out) const {
    if (fast_) {
      *out = NumberOr(self_->GetInternalField(kEventsCount).As<v8::Value>(), 0);
      return true;
    }
    v8::Local<v8::Value> value;
    if (!GetProp(s, self_, s.Key(kKeyEventsCount), &value))
      return false;
    *out = NumberOr(value, 0);
    return true;
  }
  [[nodiscard]] bool SetEventsCount(const State& s, double count) const {
    v8::Local<v8::Value> value = v8::Number::New(s.isolate(), count);
    if (fast_) {
      self_->SetInternalField(kEventsCount, value);
      return true;
    }
    return SetProp(s, self_, s.Key(kKeyEventsCount), value);
  }
  [[nodiscard]] bool AddEventsCount(const State& s,
                                    double delta,
                                    double* out) const {
    double count;
    if (!GetEventsCount(s, &count))
      return false;
    *out = count + delta;
    return SetEventsCount(s, *out);
  }
  [[nodiscard]] bool GetMaxListenersValue(const State& s,
                                          v8::Local<v8::Value>* out) const {
    if (fast_) {
      *out = self_->GetInternalField(kMaxListeners).As<v8::Value>();
      return true;
    }
    return GetProp(s, self_, s.Key(kKeyMaxListeners), out);
  }
  [[nodiscard]] bool SetMaxListenersValue(const State& s,
                                          v8::Local<v8::Value> value) const {
    if (fast_) {
      self_->SetInternalField(kMaxListeners, value);
      return true;
    }
    return SetProp(s, self_, s.Key(kKeyMaxListeners), value);
  }
  // Ensures `_events` exists, creating it (and resetting the count) if not.
  [[nodiscard]] bool EnsureEvents(const State& s,
                                  v8::Local<v8::Object>* out) const {
    v8::Local<v8::Value> events;
    if (!GetEvents(s, &events))
      return false;
    if (events->IsObject()) {
      *out = events.As<v8::Object>();
      return true;
    }
    *out = NewNullProtoObject(s.isolate());
    return SetEvents(s, *out) && SetEventsCount(s, 0);
  }
  // _getMaxListeners(this)
  [[nodiscard]] bool GetMaxListeners(const State& s, double* out) const {
    v8::Local<v8::Value> value;
    if (!GetMaxListenersValue(s, &value))
      return false;
    *out = value->IsUndefined() ? NumberOr(s.Get(kDefaultMaxListeners), 10)
                                : NumberOr(value, 0);
    return true;
  }

 private:
  v8::Local<v8::Object> self_;
  bool fast_ = false;
};

// Native data properties backing `_events`, `_eventsCount` and
// `_maxListeners` on instances.
template <InstanceField field>
void InstanceFieldGetter(v8::Local<v8::Name> name,
                         const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();
  if (holder->InternalFieldCount() == kInstanceFieldCount)
    info.GetReturnValue().Set(holder->GetInternalField(field).As<v8::Value>());
}

template <InstanceField field>
void InstanceFieldSetter(v8::Local<v8::Name> name,
                         v8::Local<v8::Value> value,
                         const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  v8::Local<v8::Object> holder = info.Holder();
  if (holder->InternalFieldCount() == kInstanceFieldCount)
    holder->SetInternalField(field, value);
}

// -- Method bodies
// -------------------------------------------------------------
//
// Each public method has a *Core function taking explicit arguments so that
// internal `this.method(...)` calls can go straight to it when the method is
// not overridden (see CallEmit / CallAddListener / CallRemoveListener below).

[[nodiscard]] bool EmitCore(const State& s,
                            const Receiver& self,
                            v8::Local<v8::Value> type,
                            base::span<v8::Local<v8::Value>> args,
                            bool* result);
[[nodiscard]] bool AddListenerCore(const State& s,
                                   const Receiver& self,
                                   v8::Local<v8::Value> type,
                                   v8::Local<v8::Value> listener,
                                   bool prepend);
[[nodiscard]] bool RemoveListenerCore(const State& s,
                                      const Receiver& self,
                                      v8::Local<v8::Value> type,
                                      v8::Local<v8::Value> listener);
[[nodiscard]] bool RemoveAllListenersCore(const State& s,
                                          const Receiver& self,
                                          bool has_type,
                                          v8::Local<v8::Value> type);

// receiver[key](...argv), honouring overrides but skipping the trip through
// V8 when receiver[key] is still our own function `own`.
[[nodiscard]] bool LookupMethod(const State& s,
                                v8::Local<v8::Object> receiver,
                                Slot key,
                                Slot own,
                                v8::Local<v8::Function>* override_out) {
  v8::Local<v8::Value> fn;
  if (!receiver->Get(s.context(), s.Key(key)).ToLocal(&fn))
    return false;
  if (fn->StrictEquals(s.Get(own))) {
    *override_out = v8::Local<v8::Function>();
    return true;
  }
  if (!fn->IsFunction()) {
    ThrowTypeError(
        s.isolate(),
        base::StrCat({"this.", gin::V8ToString(s.isolate(), s.Key(key)),
                      " is not a function"}));
    return false;
  }
  *override_out = fn.As<v8::Function>();
  return true;
}

[[nodiscard]] bool CallEmit(const State& s,
                            const Receiver& self,
                            base::span<v8::Local<v8::Value>> argv) {
  v8::Local<v8::Function> fn;
  if (!LookupMethod(s, self.self(), kKeyEmit, kFnEmit, &fn))
    return false;
  if (fn.IsEmpty()) {
    bool unused;
    return EmitCore(s, self, argv[0], argv.subspan<1>(), &unused);
  }
  return !fn->Call(s.context(), self.self(), static_cast<int>(argv.size()),
                   argv.data())
              .IsEmpty();
}

[[nodiscard]] bool CallAddListener(const State& s,
                                   const Receiver& self,
                                   bool prepend,
                                   v8::Local<v8::Value> type,
                                   v8::Local<v8::Value> listener) {
  v8::Local<v8::Function> fn;
  if (!LookupMethod(s, self.self(), prepend ? kKeyPrependListener : kKeyOn,
                    prepend ? kFnPrependListener : kFnAddListener, &fn)) {
    return false;
  }
  if (fn.IsEmpty())
    return AddListenerCore(s, self, type, listener, prepend);
  v8::Local<v8::Value> argv[] = {type, listener};
  return !fn->Call(s.context(), self.self(), 2, argv).IsEmpty();
}

[[nodiscard]] bool CallRemoveListener(const State& s,
                                      const Receiver& self,
                                      v8::Local<v8::Value> type,
                                      v8::Local<v8::Value> listener) {
  v8::Local<v8::Function> fn;
  if (!LookupMethod(s, self.self(), kKeyRemoveListener, kFnRemoveListener,
                    &fn)) {
    return false;
  }
  if (fn.IsEmpty())
    return RemoveListenerCore(s, self, type, listener);
  v8::Local<v8::Value> argv[] = {type, listener};
  return !fn->Call(s.context(), self.self(), 2, argv).IsEmpty();
}

[[nodiscard]] bool CallRemoveAllListeners(const State& s,
                                          const Receiver& self,
                                          v8::Local<v8::Value> type) {
  v8::Local<v8::Function> fn;
  if (!LookupMethod(s, self.self(), kKeyRemoveAllListeners,
                    kFnRemoveAllListeners, &fn)) {
    return false;
  }
  if (fn.IsEmpty())
    return RemoveAllListenersCore(s, self, /*has_type=*/true, type);
  v8::Local<v8::Value> argv[] = {type};
  return !fn->Call(s.context(), self.self(), 1, argv).IsEmpty();
}

// -- max listener warning ----------------------------------------------------

// Reported through console.warn(), as the `events` package did; there is no
// process.emitWarning() in the contexts this runs in.
[[nodiscard]] bool WarnMaxListenersExceeded(const State& s,
                                            v8::Local<v8::Object> self,
                                            v8::Local<v8::Value> type,
                                            v8::Local<v8::Array> existing) {
  v8::Isolate* isolate = s.isolate();
  if (!SetProp(s, existing, s.Key(kKeyWarned), v8::True(isolate)))
    return false;
  std::string count = base::NumberToString(existing->Length());
  v8::Local<v8::Value> warning = v8::Exception::Error(gin::StringToV8(
      isolate,
      base::StrCat({"Possible EventEmitter memory leak detected. ", count, " ",
                    Describe(isolate, type),
                    " listeners added. Use emitter.setMaxListeners() to "
                    "increase limit"})));
  if (!SetProp(s, warning, Intern(isolate, "name"),
               gin::StringToV8(isolate, "MaxListenersExceededWarning")) ||
      !SetProp(s, warning, Intern(isolate, "emitter"), self) ||
      !SetProp(s, warning, Intern(isolate, "type"), type) ||
      !SetProp(s, warning, Intern(isolate, "count"),
               v8::Integer::NewFromUnsigned(isolate, existing->Length()))) {
    return false;
  }
  v8::Local<v8::Value> console, warn;
  if (!s.context()
           ->Global()
           ->Get(s.context(), Intern(isolate, "console"))
           .ToLocal(&console)) {
    return false;
  }
  if (!console->IsObject())
    return true;
  if (!GetProp(s, console, Intern(isolate, "warn"), &warn))
    return false;
  if (!warn->IsFunction())
    return true;
  v8::Local<v8::Value> argv[] = {warning};
  return !warn.As<v8::Function>()
              ->Call(s.context(), console, 1, argv)
              .IsEmpty();
}

// -- EventEmitter ------------------------------------------------------------

void Constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  v8::Local<v8::Object> self = info.This();
  if (info.IsConstructCall() &&
      self->InternalFieldCount() == kInstanceFieldCount) {
    self->SetInternalField(kTag, s.data());
    self->SetInternalField(kEvents, NewNullProtoObject(s.isolate()));
    self->SetInternalField(kEventsCount, v8::Integer::New(s.isolate(), 0));
    self->SetInternalField(kMaxListeners, v8::Undefined(s.isolate()));
    return;
  }

  // EventEmitter.call(obj): initialise an arbitrary object the way
  // EventEmitter.init does.
  if (!self->IsObject() || self->StrictEquals(s.context()->Global())) {
    ThrowTypeError(s.isolate(),
                   "Class constructor EventEmitter cannot be invoked without "
                   "'new'");
    return;
  }
  v8::Local<v8::Value> events, proto_events;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
    return;
  v8::Local<v8::Value> proto = self->GetPrototype();
  if (proto->IsObject() &&
      !GetProp(s, proto, s.Key(kKeyEvents), &proto_events)) {
    return;
  }
  if (events->IsUndefined() ||
      (!proto_events.IsEmpty() && events->StrictEquals(proto_events))) {
    if (!SetProp(s, self, s.Key(kKeyEvents), NewNullProtoObject(s.isolate())) ||
        !SetProp(s, self, s.Key(kKeyEventsCount),
                 v8::Integer::New(s.isolate(), 0))) {
      return;
    }
  }
  // this._maxListeners = this._maxListeners || undefined;
  v8::Local<v8::Value> max_listeners;
  if (!GetProp(s, self, s.Key(kKeyMaxListeners), &max_listeners))
    return;
  if (!max_listeners->BooleanValue(s.isolate())) {
    std::ignore =
        SetProp(s, self, s.Key(kKeyMaxListeners), v8::Undefined(s.isolate()));
  }
}

// -- setMaxListeners / getMaxListeners ---------------------------------------

[[nodiscard]] bool ValidateMaxListeners(const State& s,
                                        v8::Local<v8::Value> n,
                                        const char* name) {
  if (n->IsNumber()) {
    double value = n.As<v8::Number>()->Value();
    if (value >= 0 && !std::isnan(value))
      return true;
  }
  ThrowRangeError(
      s.isolate(),
      base::StrCat({"The value of \"", name,
                    "\" is out of range. It must be a non-negative number. "
                    "Received ",
                    Describe(s.isolate(), n), "."}));
  return false;
}

void SetMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (!Receiver::From(s, info, "setMaxListeners", &self) ||
      !ValidateMaxListeners(s, info[0], "n") ||
      !self.SetMaxListenersValue(s, info[0])) {
    return;
  }
  info.GetReturnValue().Set(self.self());
}

void GetMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  double max;
  if (!Receiver::From(s, info, "getMaxListeners", &self) ||
      !self.GetMaxListeners(s, &max)) {
    return;
  }
  info.GetReturnValue().Set(max);
}

// -- emit --------------------------------------------------------------------

bool EmitCore(const State& s,
              const Receiver& self,
              v8::Local<v8::Value> type,
              base::span<v8::Local<v8::Value>> args,
              bool* result) {
  int argc = static_cast<int>(args.size());
  v8::Local<v8::Value>* argv = args.data();
  *result = false;
  bool do_error = type->IsString() && type->StrictEquals(s.Key(kKeyError));

  v8::Local<v8::Value> events;
  if (!self.GetEvents(s, &events))
    return false;
  if (events->IsObject()) {
    if (do_error) {
      v8::Local<v8::Value> error_handler;
      if (!GetProp(s, events, s.Key(kKeyError), &error_handler))
        return false;
      do_error = error_handler->IsUndefined();
    }
  } else if (!do_error) {
    return true;
  }

  // If there is no 'error' event listener then throw.
  if (do_error) {
    v8::Local<v8::Value> er =
        args.empty() ? v8::Undefined(s.isolate()).As<v8::Value>() : args[0];
    v8::Local<v8::Value> error_ctor = s.Get(kErrorConstructor);
    if (er->IsObject() && error_ctor->IsFunction()) {
      bool is_error;
      if (!er->InstanceOf(s.context(), error_ctor.As<v8::Object>())
               .To(&is_error)) {
        return false;
      }
      if (is_error) {
        s.isolate()->ThrowException(er);  // Unhandled 'error' event
        return false;
      }
    }
    // At least give some kind of context to the user
    std::string message = "Unhandled error.";
    if (er->BooleanValue(s.isolate())) {
      v8::Local<v8::Value> er_message = v8::Undefined(s.isolate());
      if (er->IsObject() &&
          !GetProp(s, er, Intern(s.isolate(), "message"), &er_message)) {
        return false;
      }
      base::StrAppend(&message, {" (", Describe(s.isolate(), er_message), ")"});
    }
    v8::Local<v8::Value> err =
        v8::Exception::Error(gin::StringToV8(s.isolate(), message));
    if (!SetProp(s, err, Intern(s.isolate(), "context"), er))
      return false;
    s.isolate()->ThrowException(err);  // Unhandled 'error' event
    return false;
  }

  v8::Local<v8::Value> handler;
  if (!GetProp(s, events, type, &handler))
    return false;
  if (handler->IsUndefined())
    return true;

  if (handler->IsFunction()) {
    if (handler.As<v8::Function>()
            ->Call(s.context(), self.self(), argc, argv)
            .IsEmpty()) {
      return false;
    }
  } else if (handler->IsArray()) {
    // Several listeners: hand the array to the JS dispatch loop (see
    // kDispatchSource) so the listeners are called from JIT-compiled code
    // rather than through one C++ -> JS transition each.
    v8::LocalVector<v8::Value> dispatch_argv(s.isolate());
    dispatch_argv.reserve(argc + 2);
    dispatch_argv.push_back(handler);
    dispatch_argv.push_back(self.self());
    dispatch_argv.insert(dispatch_argv.end(), args.begin(), args.end());
    if (s.Get(kDispatch)
            .As<v8::Function>()
            ->Call(s.context(), v8::Undefined(s.isolate()),
                   static_cast<int>(dispatch_argv.size()), dispatch_argv.data())
            .IsEmpty()) {
      return false;
    }
  }

  *result = true;
  return true;
}

void Emit(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (!Receiver::From(s, info, "emit", &self))
    return;

  // Arguments after `type`, on the stack for the common small counts.
  int length = info.Length();
  size_t argc = length > 1 ? length - 1 : 0;
  std::array<v8::Local<v8::Value>, 8> stack;
  std::vector<v8::Local<v8::Value>> heap;
  base::span<v8::Local<v8::Value>> args;
  if (argc <= stack.size()) {
    for (int i = 1; i < length; ++i)
      stack[i - 1] = info[i];
    args = base::span(stack).first(argc);
  } else {
    heap.reserve(argc);
    for (int i = 1; i < length; ++i)
      heap.push_back(info[i]);
    args = heap;
  }

  bool result;
  if (EmitCore(s, self, info[0], args, &result))
    info.GetReturnValue().Set(result);
}

// -- addListener / prependListener -------------------------------------------

bool AddListenerCore(const State& s,
                     const Receiver& self,
                     v8::Local<v8::Value> type,
                     v8::Local<v8::Value> listener,
                     bool prepend) {
  v8::Isolate* isolate = s.isolate();
  if (!CheckListener(s, listener))
    return false;

  v8::Local<v8::Value> events_value, existing;
  v8::Local<v8::Object> events;
  if (!self.GetEvents(s, &events_value))
    return false;
  if (!events_value->IsObject()) {
    if (!self.EnsureEvents(s, &events))
      return false;
    existing = v8::Undefined(isolate);
  } else {
    events = events_value.As<v8::Object>();
    // To avoid recursion in the case that type === "newListener"! Before
    // adding it to the listeners, first emit "newListener".
    v8::Local<v8::Value> new_listener_handler;
    if (!GetProp(s, events, s.Key(kKeyNewListener), &new_listener_handler))
      return false;
    if (!new_listener_handler->IsUndefined()) {
      v8::Local<v8::Value> unwrapped;
      if (!UnwrapListener(s, listener, &unwrapped))
        return false;
      v8::Local<v8::Value> argv[] = {s.Key(kKeyNewListener), type, unwrapped};
      if (!CallEmit(s, self, argv))
        return false;
      // Re-read `events` because a newListener handler could have caused
      // this._events to be assigned to a new object.
      if (!self.EnsureEvents(s, &events))
        return false;
    }
    if (!GetProp(s, events, type, &existing))
      return false;
  }

  if (existing->IsUndefined()) {
    // Optimize the case of one listener. Don't need the extra array object.
    double count;
    return SetProp(s, events, type, listener) &&
           self.AddEventsCount(s, 1, &count);
  }

  v8::Local<v8::Array> list;
  if (existing->IsArray()) {
    list = existing.As<v8::Array>();
    uint32_t length = list->Length();
    if (prepend) {
      // existing.unshift(listener)
      for (uint32_t i = length; i > 0; --i) {
        v8::Local<v8::Value> item;
        if (!list->Get(s.context(), i - 1).ToLocal(&item) ||
            !list->Set(s.context(), i, item).IsJust()) {
          return false;
        }
      }
      if (!list->Set(s.context(), 0, listener).IsJust())
        return false;
    } else if (!list->Set(s.context(), length, listener).IsJust()) {
      return false;
    }
  } else {
    // Adding the second element, need to change to array.
    v8::Local<v8::Value> items[] = {prepend ? listener : existing,
                                    prepend ? existing : listener};
    list = v8::Array::New(isolate, items, 2);
    if (!SetProp(s, events, type, list))
      return false;
  }

  // Check for listener leak
  double max;
  if (!self.GetMaxListeners(s, &max))
    return false;
  if (max > 0 && list->Length() > max) {
    v8::Local<v8::Value> warned;
    if (!GetProp(s, list, s.Key(kKeyWarned), &warned))
      return false;
    if (!warned->BooleanValue(isolate) &&
        !WarnMaxListenersExceeded(s, self.self(), type, list)) {
      return false;
    }
  }
  return true;
}

void AddListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (Receiver::From(s, info, "addListener", &self) &&
      AddListenerCore(s, self, info[0], info[1], /*prepend=*/false)) {
    info.GetReturnValue().Set(self.self());
  }
}

void PrependListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (Receiver::From(s, info, "prependListener", &self) &&
      AddListenerCore(s, self, info[0], info[1], /*prepend=*/true)) {
    info.GetReturnValue().Set(self.self());
  }
}

// -- once / prependOnceListener ----------------------------------------------

// The shared body of every once() wrapper; `this` is the bound state object
// created by OnceWrap().
void OnceWrapper(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  v8::Local<v8::Object> state = info.This();
  if (!state->IsObject() || state->InternalFieldCount() != kOnceFieldCount)
    return;
  auto field = [&](OnceField i) {
    return state->GetInternalField(i).As<v8::Value>();
  };
  if (field(kOnceFired)->IsTrue())
    return;

  Receiver target(s, field(kOnceTarget).As<v8::Object>());
  if (!CallRemoveListener(s, target, field(kOnceType), field(kOnceWrapFn)))
    return;
  state->SetInternalField(kOnceFired, v8::True(s.isolate()));
  v8::LocalVector<v8::Value> argv(s.isolate());
  argv.reserve(info.Length());
  for (int i = 0; i < info.Length(); ++i)
    argv.push_back(info[i]);
  v8::Local<v8::Value> result;
  if (field(kOnceListener)
          .As<v8::Function>()
          ->Call(s.context(), target.self(), static_cast<int>(argv.size()),
                 argv.data())
          .ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}

// _onceWrap(target, type, listener): `onceWrapper.bind(state)` with
// `.listener = listener`, as the `events` package builds it.
[[nodiscard]] bool OnceWrap(const State& s,
                            v8::Local<v8::Object> target,
                            v8::Local<v8::Value> type,
                            v8::Local<v8::Value> listener,
                            v8::Local<v8::Function>* out) {
  v8::Local<v8::Object> state;
  if (!s.OnceStateTemplate()->NewInstance(s.context()).ToLocal(&state))
    return false;
  state->SetInternalField(kOnceTarget, target);
  state->SetInternalField(kOnceType, type);
  state->SetInternalField(kOnceListener, listener);
  state->SetInternalField(kOnceFired, v8::False(s.isolate()));

  v8::Local<v8::Value> bind_argv[] = {state};
  v8::Local<v8::Value> wrapped;
  if (!s.Get(kFunctionBind)
           .As<v8::Function>()
           ->Call(s.context(), s.Get(kOnceWrapper), 1, bind_argv)
           .ToLocal(&wrapped) ||
      !wrapped->IsFunction()) {
    return false;
  }
  state->SetInternalField(kOnceWrapFn, wrapped);
  if (!SetProp(s, wrapped, s.Key(kKeyListener), listener))
    return false;
  *out = wrapped.As<v8::Function>();
  return true;
}

void OnceImpl(const v8::FunctionCallbackInfo<v8::Value>& info,
              const char* method,
              bool prepend) {
  State s(info);
  Receiver self;
  if (!Receiver::From(s, info, method, &self))
    return;
  v8::Local<v8::Value> type = info[0];
  v8::Local<v8::Value> listener = info[1];
  v8::Local<v8::Function> wrapper;
  if (CheckListener(s, listener) &&
      OnceWrap(s, self.self(), type, listener, &wrapper) &&
      CallAddListener(s, self, prepend, type, wrapper)) {
    info.GetReturnValue().Set(self.self());
  }
}

void Once(const v8::FunctionCallbackInfo<v8::Value>& info) {
  OnceImpl(info, "once", /*prepend=*/false);
}

void PrependOnceListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  OnceImpl(info, "prependOnceListener", /*prepend=*/true);
}

// -- removeListener ----------------------------------------------------------

// One listener for `type` is going away: drop the key. lib/events.js swaps in
// a fresh `_events` object when the last one goes to keep the object in fast
// mode; ours is a dictionary either way, so deleting is cheaper and the
// difference is not observable through the API.
[[nodiscard]] bool DropEvent(const State& s,
                             const Receiver& self,
                             v8::Local<v8::Value> events,
                             v8::Local<v8::Value> type) {
  // Unlike Get/Set, v8::Object::Delete must not run script, so it cannot
  // convert an arbitrary value (an object with a toString) to a property key
  // itself.
  if (!type->IsName() && !type->IsNumber()) {
    v8::Local<v8::String> key;
    if (!type->ToString(s.context()).ToLocal(&key))
      return false;
    type = key;
  }
  double count;
  return self.AddEventsCount(s, -1, &count) &&
         !events.As<v8::Object>()->Delete(s.context(), type).IsNothing();
}

bool RemoveListenerCore(const State& s,
                        const Receiver& self,
                        v8::Local<v8::Value> type,
                        v8::Local<v8::Value> listener) {
  v8::Isolate* isolate = s.isolate();
  if (!CheckListener(s, listener))
    return false;

  v8::Local<v8::Value> events, list;
  if (!self.GetEvents(s, &events))
    return false;
  if (!events->IsObject())
    return true;
  if (!GetProp(s, events, type, &list))
    return false;
  if (list->IsUndefined())
    return true;

  v8::Local<v8::Value> remove_handler;
  if (!GetProp(s, events, s.Key(kKeyRemoveListener), &remove_handler))
    return false;
  bool emit_remove = !remove_handler->IsUndefined();

  bool matches;
  if (!MatchesListener(s, list, listener, &matches))
    return false;
  if (matches) {
    if (!DropEvent(s, self, events, type))
      return false;
    if (emit_remove) {
      v8::Local<v8::Value> original;
      if (!UnwrapListener(s, list, &original))
        return false;
      v8::Local<v8::Value> argv[] = {s.Key(kKeyRemoveListener), type, original};
      return CallEmit(s, self, argv);
    }
    return true;
  }
  if (!list->IsArray())
    return true;

  // Search from the back without copying the list, so that removing the
  // most recently added listener (what once() wrappers and
  // removeAllListeners() do) stays cheap however long the list is.
  v8::Local<v8::Array> arr = list.As<v8::Array>();
  uint32_t length = arr->Length();
  uint32_t position = length;
  v8::Local<v8::Value> matched;
  for (uint32_t i = length; i > 0; --i) {
    v8::Local<v8::Value> item;
    if (!arr->Get(s.context(), i - 1).ToLocal(&item) ||
        !MatchesListener(s, item, listener, &matches)) {
      return false;
    }
    if (matches) {
      position = i - 1;
      matched = item;
      break;
    }
  }
  if (position == length)
    return true;

  // spliceOne(list, position), in place so the array (and its `warned`
  // marker) stays the same object.
  uint32_t new_length = length - 1;
  for (uint32_t i = position; i < new_length; ++i) {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Value> next;
    if (!arr->Get(s.context(), i + 1).ToLocal(&next) ||
        !arr->Set(s.context(), i, next).IsJust()) {
      return false;
    }
  }
  if (!SetProp(s, arr, s.Key(kKeyLength),
               v8::Integer::NewFromUnsigned(isolate, new_length))) {
    return false;
  }
  if (new_length == 1) {
    v8::Local<v8::Value> remaining;
    if (!arr->Get(s.context(), 0).ToLocal(&remaining) ||
        !SetProp(s, events, type, remaining)) {
      return false;
    }
  }

  if (emit_remove) {
    // Report the listener that was registered, not a once() wrapper.
    v8::Local<v8::Value> original;
    if (!UnwrapListener(s, matched, &original))
      return false;
    v8::Local<v8::Value> argv[] = {s.Key(kKeyRemoveListener), type, original};
    return CallEmit(s, self, argv);
  }
  return true;
}

void RemoveListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (Receiver::From(s, info, "removeListener", &self) &&
      RemoveListenerCore(s, self, info[0], info[1])) {
    info.GetReturnValue().Set(self.self());
  }
}

// -- removeAllListeners ------------------------------------------------------

// Reflect.ownKeys(object)
[[nodiscard]] bool OwnKeys(const State& s,
                           v8::Local<v8::Value> object,
                           v8::Local<v8::Array>* out) {
  return object.As<v8::Object>()
      ->GetPropertyNames(s.context(), v8::KeyCollectionMode::kOwnOnly,
                         v8::ALL_PROPERTIES, v8::IndexFilter::kIncludeIndices,
                         v8::KeyConversionMode::kConvertToString)
      .ToLocal(out);
}

bool RemoveAllListenersCore(const State& s,
                            const Receiver& self,
                            bool has_type,
                            v8::Local<v8::Value> type) {
  v8::Isolate* isolate = s.isolate();
  v8::Local<v8::Value> events;
  if (!self.GetEvents(s, &events))
    return false;
  if (!events->IsObject())
    return true;

  v8::Local<v8::Value> remove_handler;
  if (!GetProp(s, events, s.Key(kKeyRemoveListener), &remove_handler))
    return false;

  // Not listening for removeListener, no need to emit
  if (remove_handler->IsUndefined()) {
    if (!has_type)
      return self.SetEvents(s, NewNullProtoObject(isolate)) &&
             self.SetEventsCount(s, 0);
    v8::Local<v8::Value> existing;
    if (!GetProp(s, events, type, &existing))
      return false;
    return existing->IsUndefined() || DropEvent(s, self, events, type);
  }

  // Emit removeListener for all listeners on all events
  if (!has_type) {
    v8::Local<v8::Array> keys;
    v8::LocalVector<v8::Value> names(isolate);
    if (!OwnKeys(s, events, &keys) || !ReadArray(s, keys, &names))
      return false;
    for (auto& key : names) {
      v8::HandleScope handle_scope(isolate);
      if (key->StrictEquals(s.Key(kKeyRemoveListener)))
        continue;
      if (!CallRemoveAllListeners(s, self, key))
        return false;
    }
    return CallRemoveAllListeners(s, self, s.Key(kKeyRemoveListener)) &&
           self.SetEvents(s, NewNullProtoObject(isolate)) &&
           self.SetEventsCount(s, 0);
  }

  v8::Local<v8::Value> listeners;
  if (!GetProp(s, events, type, &listeners))
    return false;
  if (listeners->IsFunction())
    return CallRemoveListener(s, self, type, listeners);
  if (listeners->IsArray()) {
    // LIFO order
    v8::LocalVector<v8::Value> items(isolate);
    if (!ReadArray(s, listeners.As<v8::Array>(), &items))
      return false;
    for (size_t i = items.size(); i > 0; --i) {
      v8::HandleScope handle_scope(isolate);
      if (!CallRemoveListener(s, self, type, items[i - 1]))
        return false;
    }
  }
  return true;
}

void RemoveAllListeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (Receiver::From(s, info, "removeAllListeners", &self) &&
      RemoveAllListenersCore(s, self, info.Length() > 0, info[0])) {
    info.GetReturnValue().Set(self.self());
  }
}

// -- listeners / rawListeners / listenerCount / eventNames -------------------

void ListenersImpl(const v8::FunctionCallbackInfo<v8::Value>& info,
                   const char* method,
                   bool unwrap) {
  State s(info);
  v8::Isolate* isolate = s.isolate();
  Receiver self;
  if (!Receiver::From(s, info, method, &self))
    return;
  v8::Local<v8::Value> type = info[0];

  v8::LocalVector<v8::Value> result(isolate);
  v8::Local<v8::Value> events, evlistener;
  if (!self.GetEvents(s, &events))
    return;
  if (events->IsObject()) {
    if (!GetProp(s, events, type, &evlistener))
      return;
    if (evlistener->IsArray()) {
      if (!ReadArray(s, evlistener.As<v8::Array>(), &result))
        return;
    } else if (!evlistener->IsUndefined()) {
      result.push_back(evlistener);
    }
    if (unwrap) {
      for (auto& entry : result) {
        if (!UnwrapListener(s, entry, &entry))
          return;
      }
    }
  }
  info.GetReturnValue().Set(
      v8::Array::New(isolate, result.data(), result.size()));
}

void Listeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  ListenersImpl(info, "listeners", /*unwrap=*/true);
}

void RawListeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  ListenersImpl(info, "rawListeners", /*unwrap=*/false);
}

void ListenerCount(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (!Receiver::From(s, info, "listenerCount", &self))
    return;
  v8::Local<v8::Value> events, evlistener;
  if (!self.GetEvents(s, &events))
    return;
  uint32_t count = 0;
  if (events->IsObject()) {
    if (!GetProp(s, events, info[0], &evlistener))
      return;
    if (evlistener->IsFunction()) {
      count = 1;
    } else if (evlistener->IsArray()) {
      count = evlistener.As<v8::Array>()->Length();
    }
  }
  info.GetReturnValue().Set(count);
}

void EventNames(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  Receiver self;
  if (!Receiver::From(s, info, "eventNames", &self))
    return;
  double count;
  v8::Local<v8::Value> events;
  v8::Local<v8::Array> keys;
  if (!self.GetEventsCount(s, &count))
    return;
  if (count > 0) {
    if (!self.GetEvents(s, &events))
      return;
    if (events->IsObject()) {
      if (!OwnKeys(s, events, &keys))
        return;
      info.GetReturnValue().Set(keys);
      return;
    }
  }
  info.GetReturnValue().Set(v8::Array::New(s.isolate(), 0));
}

// -- EventEmitter.defaultMaxListeners ----------------------------------------

void DefaultMaxListenersGetter(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  info.GetReturnValue().Set(s.Get(kDefaultMaxListeners));
}

void DefaultMaxListenersSetter(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  if (!ValidateMaxListeners(s, info[0], "defaultMaxListeners"))
    return;
  s.Set(kDefaultMaxListeners, info[0]);
}

// -- Template assembly -------------------------------------------------------

// The one piece of JavaScript in here: calling N listeners from a JS loop
// costs a fraction of N v8::Function::Call() entries from C++, because the
// calls get inlined into the loop. Iterates over a copy, like lib/events.js,
// so listeners added or removed by a listener do not affect this emit, and
// captures Reflect.apply before any page script can replace it.
constexpr char kDispatchSource[] =
    "(() => {"
    "  const apply = Reflect.apply;"
    "  return function emitMany(list, self, ...args) {"
    "    const n = list.length;"
    "    const listeners = new Array(n);"
    "    for (let i = 0; i < n; i++) listeners[i] = list[i];"
    "    for (let i = 0; i < n; i++) apply(listeners[i], self, args);"
    "  };"
    "})()";

v8::Local<v8::Function> CompileDispatch(v8::Local<v8::Context> context) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::ScriptOrigin origin(
      gin::StringToV8(isolate, "electron/js2c/event_emitter_dispatch"));
  v8::Local<v8::Script> script =
      v8::Script::Compile(context, gin::StringToV8(isolate, kDispatchSource),
                          &origin)
          .ToLocalChecked();
  return script->Run(context).ToLocalChecked().As<v8::Function>();
}

v8::Local<v8::FunctionTemplate> Method(v8::Isolate* isolate,
                                       v8::FunctionCallback callback,
                                       v8::Local<v8::Value> data,
                                       std::string_view name,
                                       int length) {
  v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(
      isolate, callback, data, v8::Local<v8::Signature>(), length,
      v8::ConstructorBehavior::kThrow);
  tmpl->SetClassName(Intern(isolate, name));
  return tmpl;
}

}  // namespace

v8::Local<v8::Function> CreateNodeEventEmitterConstructor(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::ObjectTemplate> data_template =
      v8::ObjectTemplate::New(isolate);
  data_template->SetInternalFieldCount(kSlotCount);
  v8::Local<v8::Object> data =
      data_template->NewInstance(context).ToLocalChecked();
  data->SetInternalField(kMagic, v8::Integer::New(isolate, kMagicValue));
  data->SetInternalField(kDefaultMaxListeners, v8::Integer::New(isolate, 10));

  // Captured before any page script runs, so later tampering with the
  // globals does not affect the emitter.
  v8::Local<v8::Value> error_ctor, function_ctor, function_proto, bind;
  v8::Local<v8::Object> global = context->Global();
  if (!global->Get(context, Intern(isolate, "Error")).ToLocal(&error_ctor))
    error_ctor = v8::Undefined(isolate);
  CHECK(global->Get(context, Intern(isolate, "Function"))
            .ToLocal(&function_ctor) &&
        function_ctor->IsObject() &&
        function_ctor.As<v8::Object>()
            ->Get(context, Intern(isolate, "prototype"))
            .ToLocal(&function_proto) &&
        function_proto->IsObject() &&
        function_proto.As<v8::Object>()
            ->Get(context, Intern(isolate, "bind"))
            .ToLocal(&bind) &&
        bind->IsFunction());
  data->SetInternalField(kErrorConstructor, error_ctor);
  data->SetInternalField(kFunctionBind, bind);
  data->SetInternalField(kDispatch, CompileDispatch(context));

  v8::Local<v8::ObjectTemplate> once_state_template =
      v8::ObjectTemplate::New(isolate);
  once_state_template->SetInternalFieldCount(kOnceFieldCount);
  data->SetInternalField(kOnceStateTemplate, once_state_template);
  v8::Local<v8::FunctionTemplate> once_wrapper = v8::FunctionTemplate::New(
      isolate, OnceWrapper, data, v8::Local<v8::Signature>(), 0,
      v8::ConstructorBehavior::kAllow);
  once_wrapper->SetClassName(Intern(isolate, "onceWrapper"));
  data->SetInternalField(kOnceWrapper,
                         once_wrapper->GetFunction(context).ToLocalChecked());

  data->SetInternalField(kKeyEvents, Intern(isolate, "_events"));
  data->SetInternalField(kKeyEventsCount, Intern(isolate, "_eventsCount"));
  data->SetInternalField(kKeyMaxListeners, Intern(isolate, "_maxListeners"));
  data->SetInternalField(kKeyListener, Intern(isolate, "listener"));
  data->SetInternalField(kKeyError, Intern(isolate, "error"));
  data->SetInternalField(kKeyNewListener, Intern(isolate, "newListener"));
  data->SetInternalField(kKeyRemoveListener, Intern(isolate, "removeListener"));
  data->SetInternalField(kKeyWarned, Intern(isolate, "warned"));
  data->SetInternalField(kKeyLength, Intern(isolate, "length"));
  data->SetInternalField(kKeyEmit, Intern(isolate, "emit"));
  data->SetInternalField(kKeyOn, Intern(isolate, "on"));
  data->SetInternalField(kKeyPrependListener,
                         Intern(isolate, "prependListener"));
  data->SetInternalField(kKeyRemoveAllListeners,
                         Intern(isolate, "removeAllListeners"));

  v8::Local<v8::FunctionTemplate> ctor = v8::FunctionTemplate::New(
      isolate, Constructor, data, v8::Local<v8::Signature>(), 0);
  ctor->SetClassName(Intern(isolate, "EventEmitter"));

  // Instances carry their state in internal fields, surfaced as the usual own
  // `_events` / `_eventsCount` / `_maxListeners` data properties. V8 lays
  // template properties out in reverse, so declare them backwards to match
  // lib/events.js's property order.
  v8::Local<v8::ObjectTemplate> instance = ctor->InstanceTemplate();
  instance->SetInternalFieldCount(kInstanceFieldCount);
  instance->SetNativeDataProperty(
      Intern(isolate, "_maxListeners"), InstanceFieldGetter<kMaxListeners>,
      InstanceFieldSetter<kMaxListeners>, data, v8::None,
      v8::SideEffectType::kHasNoSideEffect);
  instance->SetNativeDataProperty(
      Intern(isolate, "_eventsCount"), InstanceFieldGetter<kEventsCount>,
      InstanceFieldSetter<kEventsCount>, data, v8::None,
      v8::SideEffectType::kHasNoSideEffect);
  instance->SetNativeDataProperty(Intern(isolate, "_events"),
                                  InstanceFieldGetter<kEvents>,
                                  InstanceFieldSetter<kEvents>, data, v8::None,
                                  v8::SideEffectType::kHasNoSideEffect);

  // Prototype, in lib/events.js declaration order. The methods are plain
  // enumerable data properties there, so they are here too.
  v8::Local<v8::ObjectTemplate> proto = ctor->PrototypeTemplate();
  proto->Set(Intern(isolate, "_events"), v8::Undefined(isolate));
  proto->Set(Intern(isolate, "_eventsCount"), v8::Integer::New(isolate, 0));
  proto->Set(Intern(isolate, "_maxListeners"), v8::Undefined(isolate));
  proto->Set(Intern(isolate, "setMaxListeners"),
             Method(isolate, SetMaxListeners, data, "setMaxListeners", 1));
  proto->Set(Intern(isolate, "getMaxListeners"),
             Method(isolate, GetMaxListeners, data, "getMaxListeners", 0));
  proto->Set(Intern(isolate, "emit"), Method(isolate, Emit, data, "emit", 1));
  v8::Local<v8::FunctionTemplate> add_listener =
      Method(isolate, AddListener, data, "addListener", 2);
  proto->Set(Intern(isolate, "addListener"), add_listener);
  proto->Set(Intern(isolate, "on"), add_listener);
  proto->Set(Intern(isolate, "prependListener"),
             Method(isolate, PrependListener, data, "prependListener", 2));
  proto->Set(Intern(isolate, "once"), Method(isolate, Once, data, "once", 2));
  proto->Set(
      Intern(isolate, "prependOnceListener"),
      Method(isolate, PrependOnceListener, data, "prependOnceListener", 2));
  v8::Local<v8::FunctionTemplate> remove_listener =
      Method(isolate, RemoveListener, data, "removeListener", 2);
  proto->Set(Intern(isolate, "removeListener"), remove_listener);
  proto->Set(Intern(isolate, "off"), remove_listener);
  proto->Set(
      Intern(isolate, "removeAllListeners"),
      Method(isolate, RemoveAllListeners, data, "removeAllListeners", 1));
  proto->Set(Intern(isolate, "listeners"),
             Method(isolate, Listeners, data, "listeners", 1));
  proto->Set(Intern(isolate, "rawListeners"),
             Method(isolate, RawListeners, data, "rawListeners", 1));
  proto->Set(Intern(isolate, "listenerCount"),
             Method(isolate, ListenerCount, data, "listenerCount", 2));
  proto->Set(Intern(isolate, "eventNames"),
             Method(isolate, EventNames, data, "eventNames", 0));

  ctor->SetAccessorProperty(
      Intern(isolate, "defaultMaxListeners"),
      Method(isolate, DefaultMaxListenersGetter, data, "get", 0),
      Method(isolate, DefaultMaxListenersSetter, data, "set", 1),
      v8::DontDelete);

  v8::Local<v8::Function> fn = ctor->GetFunction(context).ToLocalChecked();

  // Remember our own method functions for the direct-dispatch check.
  v8::Local<v8::Value> prototype;
  CHECK(fn->Get(context, Intern(isolate, "prototype")).ToLocal(&prototype) &&
        prototype->IsObject());
  auto remember = [&](Slot slot, const char* name) {
    v8::Local<v8::Value> method;
    CHECK(prototype.As<v8::Object>()
              ->Get(context, Intern(isolate, name))
              .ToLocal(&method) &&
          method->IsFunction());
    data->SetInternalField(slot, method);
  };
  remember(kFnEmit, "emit");
  remember(kFnAddListener, "addListener");
  remember(kFnPrependListener, "prependListener");
  remember(kFnRemoveListener, "removeListener");
  remember(kFnRemoveAllListeners, "removeAllListeners");

  return handle_scope.Escape(fn);
}

v8::Local<v8::Function> GetNodeEventEmitterConstructor(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Private> key =
      v8::Private::ForApi(isolate, Intern(isolate, "electron:EventEmitter"));
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> existing;
  if (global->GetPrivate(context, key).ToLocal(&existing) &&
      existing->IsFunction()) {
    return existing.As<v8::Function>();
  }
  v8::Local<v8::Function> ctor = CreateNodeEventEmitterConstructor(context);
  global->SetPrivate(context, key, ctor).Check();
  return ctor;
}

v8::Local<v8::Object> NewNodeEventEmitter(v8::Local<v8::Context> context) {
  return GetNodeEventEmitterConstructor(context)
      ->NewInstance(context, 0, nullptr)
      .ToLocalChecked();
}

bool EmitEvent(v8::Isolate* isolate,
               v8::Local<v8::Object> emitter,
               v8::Local<v8::Value> type,
               base::span<v8::Local<v8::Value>> args) {
  if (emitter->InternalFieldCount() == kInstanceFieldCount) {
    v8::Local<v8::Value> tag = emitter->GetInternalField(kTag).As<v8::Value>();
    if (IsStateData(tag)) {
      State s(isolate, tag);
      bool unused;
      return EmitCore(s, Receiver(s, emitter), type, args, &unused);
    }
  }
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> emit;
  if (!emitter->Get(context, gin::StringToSymbol(isolate, "emit"))
           .ToLocal(&emit)) {
    return false;
  }
  if (!emit->IsFunction())
    return true;
  v8::LocalVector<v8::Value> argv(isolate);
  argv.reserve(args.size() + 1);
  argv.push_back(type);
  argv.insert(argv.end(), args.begin(), args.end());
  return !emit.As<v8::Function>()
              ->Call(context, emitter, static_cast<int>(argv.size()),
                     argv.data())
              .IsEmpty();
}

}  // namespace gin_helper
