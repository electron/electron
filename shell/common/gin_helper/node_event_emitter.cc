// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/node_event_emitter.h"

#include <cmath>
#include <string>
#include <string_view>
#include <tuple>

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
#include "v8/include/v8-template.h"

// A C++ port of the `EventEmitter` class from Node.js's lib/events.js,
// limited to the instance API that the `events` package previously provided
// to sandboxed preloads. Function names follow lib/events.js so the two can be
// read side by side.
//
// Every V8 call that can run script is checked; on failure (a pending
// exception or termination) the callback returns without touching V8 again so
// the exception propagates to the caller unchanged.

namespace gin_helper {

namespace {

// -- Per-context state -------------------------------------------------------

// Internal field slots of the object passed as callback data to every
// function created here: the mutable class-wide default, the realm's Error
// constructor for `instanceof Error`, pre-internalised property keys, and the
// template for once() wrappers' bound state.
enum Slot {
  kDefaultMaxListeners = 0,
  kErrorConstructor,
  kOnceStateTemplate,
  kKeyEvents,          // "_events"
  kKeyEventsCount,     // "_eventsCount"
  kKeyMaxListeners,    // "_maxListeners"
  kKeyListener,        // "listener"
  kKeyError,           // "error"
  kKeyNewListener,     // "newListener"
  kKeyRemoveListener,  // "removeListener"
  kKeyWarned,          // "warned"
  kKeyLength,          // "length"
  kSlotCount,
};

class State {
  STACK_ALLOCATED();

 public:
  explicit State(const v8::FunctionCallbackInfo<v8::Value>& info)
      : isolate_(info.GetIsolate()),
        context_(isolate_->GetCurrentContext()),
        data_(info.Data().As<v8::Object>()) {}

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const { return context_; }

  v8::Local<v8::Value> Get(Slot slot) const {
    return data_->GetInternalField(slot).As<v8::Value>();
  }
  v8::Local<v8::String> Key(Slot slot) const {
    return Get(slot).As<v8::String>();
  }
  v8::Local<v8::ObjectTemplate> OnceStateTemplate() const {
    return data_->GetInternalField(kOnceStateTemplate).As<v8::ObjectTemplate>();
  }
  void SetDefaultMaxListeners(v8::Local<v8::Value> value) {
    data_->SetInternalField(kDefaultMaxListeners, value);
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

// Property access on a value already known to be an object.
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

// The methods are strict-mode functions in lib/events.js, so calling one
// detached (`const { on } = emitter; on('x', f)`) throws instead of touching
// the global object. API callbacks receive the global proxy for an undefined
// receiver, so treat that the same as a non-object receiver.
[[nodiscard]] bool GetReceiver(const State& s,
                               const v8::FunctionCallbackInfo<v8::Value>& info,
                               const char* method,
                               v8::Local<v8::Object>* out) {
  v8::Local<v8::Value> receiver = info.This();
  if (!receiver->IsObject() || receiver->StrictEquals(s.context()->Global())) {
    ThrowTypeError(
        s.isolate(),
        base::StrCat({"EventEmitter.prototype.", method,
                      " called on undefined or a non-object receiver"}));
    return false;
  }
  *out = receiver.As<v8::Object>();
  return true;
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

// Calls receiver[method](...argv) so that subclass overrides are honoured,
// as the `this.emit(...)` / `this.removeListener(...)` calls in lib/events.js
// are.
[[nodiscard]] bool CallMethod(const State& s,
                              v8::Local<v8::Object> receiver,
                              std::string_view method,
                              base::span<v8::Local<v8::Value>> argv) {
  v8::Local<v8::Value> fn;
  if (!receiver->Get(s.context(), Intern(s.isolate(), method)).ToLocal(&fn))
    return false;
  if (!fn->IsFunction()) {
    ThrowTypeError(s.isolate(),
                   base::StrCat({"this.", method, " is not a function"}));
    return false;
  }
  return !fn.As<v8::Function>()
              ->Call(s.context(), receiver, static_cast<int>(argv.size()),
                     argv.data())
              .IsEmpty();
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
  out->reserve(out->size() + array->Length());
  for (uint32_t i = 0; i < array->Length(); ++i) {
    v8::Local<v8::Value> item;
    if (!array->Get(s.context(), i).ToLocal(&item))
      return false;
    out->push_back(item);
  }
  return true;
}

// -- _getMaxListeners / warning ----------------------------------------------

[[nodiscard]] bool GetMaxListenersOf(const State& s,
                                     v8::Local<v8::Object> self,
                                     double* out) {
  v8::Local<v8::Value> value;
  if (!GetProp(s, self, s.Key(kKeyMaxListeners), &value))
    return false;
  *out = value->IsUndefined() ? NumberOr(s.Get(kDefaultMaxListeners), 10)
                              : NumberOr(value, 0);
  return true;
}

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
  v8::Local<v8::Value> receiver = info.This();
  if (!info.IsConstructCall() &&
      (!receiver->IsObject() ||
       receiver->StrictEquals(s.context()->Global()))) {
    ThrowTypeError(s.isolate(),
                   "Class constructor EventEmitter cannot be invoked without "
                   "'new'");
    return;
  }
  v8::Local<v8::Object> self = receiver.As<v8::Object>();

  // EventEmitter.init
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
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, "setMaxListeners", &self) ||
      !ValidateMaxListeners(s, info[0], "n") ||
      !SetProp(s, self, s.Key(kKeyMaxListeners), info[0])) {
    return;
  }
  info.GetReturnValue().Set(self);
}

void GetMaxListeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  v8::Local<v8::Object> self;
  double max;
  if (!GetReceiver(s, info, "getMaxListeners", &self) ||
      !GetMaxListenersOf(s, self, &max)) {
    return;
  }
  info.GetReturnValue().Set(max);
}

// -- emit --------------------------------------------------------------------

void Emit(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, "emit", &self))
    return;
  v8::Local<v8::Value> type = info[0];

  v8::LocalVector<v8::Value> args(s.isolate());
  args.reserve(info.Length() > 1 ? info.Length() - 1 : 0);
  for (int i = 1; i < info.Length(); ++i)
    args.push_back(info[i]);
  int argc = static_cast<int>(args.size());

  bool do_error = type->IsString() && type->StrictEquals(s.Key(kKeyError));

  v8::Local<v8::Value> events;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
    return;
  if (events->IsObject()) {
    if (do_error) {
      v8::Local<v8::Value> error_handler;
      if (!GetProp(s, events, s.Key(kKeyError), &error_handler))
        return;
      do_error = error_handler->IsUndefined();
    }
  } else if (!do_error) {
    info.GetReturnValue().Set(false);
    return;
  }

  // If there is no 'error' event listener then throw.
  if (do_error) {
    v8::Local<v8::Value> er =
        argc > 0 ? args[0] : v8::Undefined(s.isolate()).As<v8::Value>();
    v8::Local<v8::Value> error_ctor = s.Get(kErrorConstructor);
    if (er->IsObject() && error_ctor->IsFunction()) {
      bool is_error;
      if (!er->InstanceOf(s.context(), error_ctor.As<v8::Object>())
               .To(&is_error)) {
        return;
      }
      if (is_error) {
        s.isolate()->ThrowException(er);  // Unhandled 'error' event
        return;
      }
    }
    // At least give some kind of context to the user
    std::string message = "Unhandled error.";
    if (er->BooleanValue(s.isolate())) {
      v8::Local<v8::Value> er_message = v8::Undefined(s.isolate());
      if (er->IsObject() &&
          !GetProp(s, er, Intern(s.isolate(), "message"), &er_message)) {
        return;
      }
      base::StrAppend(&message, {" (", Describe(s.isolate(), er_message), ")"});
    }
    v8::Local<v8::Value> err =
        v8::Exception::Error(gin::StringToV8(s.isolate(), message));
    if (!SetProp(s, err, Intern(s.isolate(), "context"), er))
      return;
    s.isolate()->ThrowException(err);  // Unhandled 'error' event
    return;
  }

  v8::Local<v8::Value> handler;
  if (!GetProp(s, events, type, &handler))
    return;
  if (handler->IsUndefined()) {
    info.GetReturnValue().Set(false);
    return;
  }

  if (handler->IsFunction()) {
    if (handler.As<v8::Function>()
            ->Call(s.context(), self, argc, args.data())
            .IsEmpty()) {
      return;
    }
  } else if (handler->IsArray()) {
    // Iterate over a copy so listeners added or removed by a listener do not
    // affect this emit.
    v8::LocalVector<v8::Value> listeners(s.isolate());
    if (!ReadArray(s, handler.As<v8::Array>(), &listeners))
      return;
    for (auto& fn : listeners) {
      if (fn->IsFunction() && fn.As<v8::Function>()
                                  ->Call(s.context(), self, argc, args.data())
                                  .IsEmpty()) {
        return;
      }
    }
  }

  info.GetReturnValue().Set(true);
}

// -- addListener / prependListener -------------------------------------------

void AddListenerImpl(const v8::FunctionCallbackInfo<v8::Value>& info,
                     const char* method,
                     bool prepend) {
  State s(info);
  v8::Isolate* isolate = s.isolate();
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, method, &self))
    return;
  v8::Local<v8::Value> type = info[0];
  v8::Local<v8::Value> listener = info[1];
  if (!CheckListener(s, listener))
    return;

  v8::Local<v8::Value> events, existing;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
    return;
  if (!events->IsObject()) {
    events = NewNullProtoObject(isolate);
    if (!SetProp(s, self, s.Key(kKeyEvents), events) ||
        !SetProp(s, self, s.Key(kKeyEventsCount),
                 v8::Integer::New(isolate, 0))) {
      return;
    }
    existing = v8::Undefined(isolate);
  } else {
    // To avoid recursion in the case that type === "newListener"! Before
    // adding it to the listeners, first emit "newListener".
    v8::Local<v8::Value> new_listener_handler;
    if (!GetProp(s, events, s.Key(kKeyNewListener), &new_listener_handler))
      return;
    if (!new_listener_handler->IsUndefined()) {
      v8::Local<v8::Value> unwrapped;
      if (!UnwrapListener(s, listener, &unwrapped))
        return;
      v8::Local<v8::Value> argv[] = {s.Key(kKeyNewListener), type, unwrapped};
      if (!CallMethod(s, self, "emit", argv))
        return;
      // Re-assign `events` because a newListener handler could have caused
      // this._events to be assigned to a new object.
      if (!GetProp(s, self, s.Key(kKeyEvents), &events))
        return;
      if (!events->IsObject()) {
        events = NewNullProtoObject(isolate);
        if (!SetProp(s, self, s.Key(kKeyEvents), events) ||
            !SetProp(s, self, s.Key(kKeyEventsCount),
                     v8::Integer::New(isolate, 0))) {
          return;
        }
      }
    }
    if (!GetProp(s, events, type, &existing))
      return;
  }

  if (existing->IsUndefined()) {
    // Optimize the case of one listener. Don't need the extra array object.
    v8::Local<v8::Value> count;
    if (!SetProp(s, events, type, listener) ||
        !GetProp(s, self, s.Key(kKeyEventsCount), &count) ||
        !SetProp(s, self, s.Key(kKeyEventsCount),
                 v8::Number::New(isolate, NumberOr(count, 0) + 1))) {
      return;
    }
  } else {
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
            return;
          }
        }
        if (!list->Set(s.context(), 0, listener).IsJust())
          return;
      } else if (!list->Set(s.context(), length, listener).IsJust()) {
        return;
      }
    } else {
      // Adding the second element, need to change to array.
      v8::Local<v8::Value> items[] = {prepend ? listener : existing,
                                      prepend ? existing : listener};
      list = v8::Array::New(isolate, items, 2);
      if (!SetProp(s, events, type, list))
        return;
    }

    // Check for listener leak
    double max;
    if (!GetMaxListenersOf(s, self, &max))
      return;
    if (max > 0 && list->Length() > max) {
      v8::Local<v8::Value> warned;
      if (!GetProp(s, list, s.Key(kKeyWarned), &warned))
        return;
      if (!warned->BooleanValue(isolate) &&
          !WarnMaxListenersExceeded(s, self, type, list)) {
        return;
      }
    }
  }

  info.GetReturnValue().Set(self);
}

void AddListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  AddListenerImpl(info, "addListener", /*prepend=*/false);
}

void PrependListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  AddListenerImpl(info, "prependListener", /*prepend=*/true);
}

// -- once / prependOnceListener ----------------------------------------------

// Bound state of a once() wrapper: [target, type, listener, wrapper, fired].
enum OnceSlot {
  kOnceTarget = 0,
  kOnceType,
  kOnceListener,
  kOnceWrapper,
  kOnceFired,
  kOnceSlotCount,
};

void OnceWrapper(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> bound = info.Data().As<v8::Object>();
  auto slot = [&](OnceSlot i) {
    return bound->GetInternalField(i).As<v8::Value>();
  };
  if (slot(kOnceFired)->IsTrue())
    return;
  bound->SetInternalField(kOnceFired, v8::True(isolate));

  v8::Local<v8::Object> target = slot(kOnceTarget).As<v8::Object>();
  v8::Local<v8::Value> remove;
  if (!target->Get(context, Intern(isolate, "removeListener"))
           .ToLocal(&remove)) {
    return;
  }
  if (remove->IsFunction()) {
    v8::Local<v8::Value> argv[] = {slot(kOnceType), slot(kOnceWrapper)};
    if (remove.As<v8::Function>()->Call(context, target, 2, argv).IsEmpty())
      return;
  }
  v8::LocalVector<v8::Value> argv(isolate);
  argv.reserve(info.Length());
  for (int i = 0; i < info.Length(); ++i)
    argv.push_back(info[i]);
  v8::Local<v8::Value> result;
  if (slot(kOnceListener)
          .As<v8::Function>()
          ->Call(context, target, static_cast<int>(argv.size()), argv.data())
          .ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}

// _onceWrap(target, type, listener)
[[nodiscard]] bool OnceWrap(const State& s,
                            v8::Local<v8::Object> target,
                            v8::Local<v8::Value> type,
                            v8::Local<v8::Value> listener,
                            v8::Local<v8::Function>* out) {
  v8::Local<v8::Object> bound;
  if (!s.OnceStateTemplate()->NewInstance(s.context()).ToLocal(&bound))
    return false;
  bound->SetInternalField(kOnceTarget, target);
  bound->SetInternalField(kOnceType, type);
  bound->SetInternalField(kOnceListener, listener);
  bound->SetInternalField(kOnceFired, v8::False(s.isolate()));
  v8::Local<v8::Function> wrapper;
  if (!v8::Function::New(s.context(), OnceWrapper, bound, 0,
                         v8::ConstructorBehavior::kThrow)
           .ToLocal(&wrapper)) {
    return false;
  }
  bound->SetInternalField(kOnceWrapper, wrapper);
  wrapper->SetName(Intern(s.isolate(), "bound onceWrapper"));
  if (!SetProp(s, wrapper, s.Key(kKeyListener), listener))
    return false;
  *out = wrapper;
  return true;
}

void OnceImpl(const v8::FunctionCallbackInfo<v8::Value>& info,
              const char* method,
              const char* add_method) {
  State s(info);
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, method, &self))
    return;
  v8::Local<v8::Value> type = info[0];
  v8::Local<v8::Value> listener = info[1];
  v8::Local<v8::Function> wrapper;
  if (!CheckListener(s, listener) ||
      !OnceWrap(s, self, type, listener, &wrapper)) {
    return;
  }
  v8::Local<v8::Value> argv[] = {type, wrapper};
  if (!CallMethod(s, self, add_method, argv))
    return;
  info.GetReturnValue().Set(self);
}

void Once(const v8::FunctionCallbackInfo<v8::Value>& info) {
  OnceImpl(info, "once", "on");
}

void PrependOnceListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  OnceImpl(info, "prependOnceListener", "prependListener");
}

// -- removeListener ----------------------------------------------------------

void RemoveListener(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  v8::Isolate* isolate = s.isolate();
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, "removeListener", &self))
    return;
  v8::Local<v8::Value> type = info[0];
  v8::Local<v8::Value> listener = info[1];
  if (!CheckListener(s, listener))
    return;

  v8::Local<v8::Value> events, list;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
    return;
  if (!events->IsObject()) {
    info.GetReturnValue().Set(self);
    return;
  }
  if (!GetProp(s, events, type, &list))
    return;
  if (list->IsUndefined()) {
    info.GetReturnValue().Set(self);
    return;
  }

  v8::Local<v8::Value> remove_handler;
  if (!GetProp(s, events, s.Key(kKeyRemoveListener), &remove_handler))
    return;
  bool emit_remove = !remove_handler->IsUndefined();

  bool matches;
  if (!MatchesListener(s, list, listener, &matches))
    return;
  if (matches) {
    v8::Local<v8::Value> count;
    if (!GetProp(s, self, s.Key(kKeyEventsCount), &count))
      return;
    double n = NumberOr(count, 0) - 1;
    if (!SetProp(s, self, s.Key(kKeyEventsCount),
                 v8::Number::New(isolate, n))) {
      return;
    }
    if (n == 0) {
      if (!SetProp(s, self, s.Key(kKeyEvents), NewNullProtoObject(isolate)))
        return;
    } else if (events.As<v8::Object>()->Delete(s.context(), type).IsNothing()) {
      return;
    }
    if (emit_remove) {
      v8::Local<v8::Value> original;
      if (!UnwrapListener(s, list, &original))
        return;
      v8::Local<v8::Value> argv[] = {s.Key(kKeyRemoveListener), type, original};
      if (!CallMethod(s, self, "emit", argv))
        return;
    }
  } else if (list->IsArray()) {
    v8::Local<v8::Array> arr = list.As<v8::Array>();
    v8::LocalVector<v8::Value> items(isolate);
    if (!ReadArray(s, arr, &items))
      return;
    int position = -1;
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
      if (!MatchesListener(s, items[i], listener, &matches))
        return;
      if (matches) {
        position = i;
        break;
      }
    }
    if (position < 0) {
      info.GetReturnValue().Set(self);
      return;
    }

    // spliceOne(list, position), in place so the array (and its `warned`
    // marker) stays the same object.
    size_t new_length = items.size() - 1;
    for (size_t i = position; i < new_length; ++i) {
      if (!arr->Set(s.context(), static_cast<uint32_t>(i), items[i + 1])
               .IsJust()) {
        return;
      }
    }
    if (!SetProp(s, arr, s.Key(kKeyLength),
                 v8::Integer::NewFromUnsigned(
                     isolate, static_cast<uint32_t>(new_length)))) {
      return;
    }
    if (new_length == 1 &&
        !SetProp(s, events, type, items[position == 0 ? 1 : 0])) {
      return;
    }

    if (emit_remove) {
      v8::Local<v8::Value> argv[] = {s.Key(kKeyRemoveListener), type, listener};
      if (!CallMethod(s, self, "emit", argv))
        return;
    }
  }

  info.GetReturnValue().Set(self);
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

void RemoveAllListeners(const v8::FunctionCallbackInfo<v8::Value>& info) {
  State s(info);
  v8::Isolate* isolate = s.isolate();
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, "removeAllListeners", &self))
    return;
  bool has_type = info.Length() > 0;
  v8::Local<v8::Value> type = info[0];

  v8::Local<v8::Value> events;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
    return;
  if (!events->IsObject()) {
    info.GetReturnValue().Set(self);
    return;
  }

  v8::Local<v8::Value> remove_handler;
  if (!GetProp(s, events, s.Key(kKeyRemoveListener), &remove_handler))
    return;

  // Not listening for removeListener, no need to emit
  if (remove_handler->IsUndefined()) {
    if (!has_type) {
      if (!SetProp(s, self, s.Key(kKeyEvents), NewNullProtoObject(isolate)) ||
          !SetProp(s, self, s.Key(kKeyEventsCount),
                   v8::Integer::New(isolate, 0))) {
        return;
      }
    } else {
      v8::Local<v8::Value> existing, count;
      if (!GetProp(s, events, type, &existing))
        return;
      if (!existing->IsUndefined()) {
        if (!GetProp(s, self, s.Key(kKeyEventsCount), &count))
          return;
        double n = NumberOr(count, 0) - 1;
        if (!SetProp(s, self, s.Key(kKeyEventsCount),
                     v8::Number::New(isolate, n))) {
          return;
        }
        if (n == 0) {
          if (!SetProp(s, self, s.Key(kKeyEvents),
                       NewNullProtoObject(isolate))) {
            return;
          }
        } else if (events.As<v8::Object>()
                       ->Delete(s.context(), type)
                       .IsNothing()) {
          return;
        }
      }
    }
    info.GetReturnValue().Set(self);
    return;
  }

  // Emit removeListener for all listeners on all events
  if (!has_type) {
    v8::Local<v8::Array> keys;
    v8::LocalVector<v8::Value> names(isolate);
    if (!OwnKeys(s, events, &keys) || !ReadArray(s, keys, &names))
      return;
    for (auto& key : names) {
      if (key->StrictEquals(s.Key(kKeyRemoveListener)))
        continue;
      v8::Local<v8::Value> argv[] = {key};
      if (!CallMethod(s, self, "removeAllListeners", argv))
        return;
    }
    v8::Local<v8::Value> argv[] = {s.Key(kKeyRemoveListener)};
    if (!CallMethod(s, self, "removeAllListeners", argv) ||
        !SetProp(s, self, s.Key(kKeyEvents), NewNullProtoObject(isolate)) ||
        !SetProp(s, self, s.Key(kKeyEventsCount),
                 v8::Integer::New(isolate, 0))) {
      return;
    }
    info.GetReturnValue().Set(self);
    return;
  }

  v8::Local<v8::Value> listeners;
  if (!GetProp(s, events, type, &listeners))
    return;
  if (listeners->IsFunction()) {
    v8::Local<v8::Value> argv[] = {type, listeners};
    if (!CallMethod(s, self, "removeListener", argv))
      return;
  } else if (listeners->IsArray()) {
    // LIFO order
    v8::LocalVector<v8::Value> items(isolate);
    if (!ReadArray(s, listeners.As<v8::Array>(), &items))
      return;
    for (size_t i = items.size(); i > 0; --i) {
      v8::Local<v8::Value> argv[] = {type, items[i - 1]};
      if (!CallMethod(s, self, "removeListener", argv))
        return;
    }
  }
  info.GetReturnValue().Set(self);
}

// -- listeners / rawListeners / listenerCount / eventNames -------------------

void ListenersImpl(const v8::FunctionCallbackInfo<v8::Value>& info,
                   const char* method,
                   bool unwrap) {
  State s(info);
  v8::Isolate* isolate = s.isolate();
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, method, &self))
    return;
  v8::Local<v8::Value> type = info[0];

  v8::LocalVector<v8::Value> result(isolate);
  v8::Local<v8::Value> events, evlistener;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
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
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, "listenerCount", &self))
    return;
  v8::Local<v8::Value> events, evlistener;
  if (!GetProp(s, self, s.Key(kKeyEvents), &events))
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
  v8::Local<v8::Object> self;
  if (!GetReceiver(s, info, "eventNames", &self))
    return;
  v8::Local<v8::Value> count, events;
  v8::Local<v8::Array> keys;
  if (!GetProp(s, self, s.Key(kKeyEventsCount), &count))
    return;
  if (NumberOr(count, 0) > 0) {
    if (!GetProp(s, self, s.Key(kKeyEvents), &events))
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
  s.SetDefaultMaxListeners(info[0]);
}

// -- Template assembly -------------------------------------------------------

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
  data->SetInternalField(kDefaultMaxListeners, v8::Integer::New(isolate, 10));
  v8::Local<v8::Value> error_ctor;
  if (!context->Global()
           ->Get(context, Intern(isolate, "Error"))
           .ToLocal(&error_ctor)) {
    error_ctor = v8::Undefined(isolate);
  }
  data->SetInternalField(kErrorConstructor, error_ctor);
  v8::Local<v8::ObjectTemplate> once_state_template =
      v8::ObjectTemplate::New(isolate);
  once_state_template->SetInternalFieldCount(kOnceSlotCount);
  data->SetInternalField(kOnceStateTemplate, once_state_template);
  data->SetInternalField(kKeyEvents, Intern(isolate, "_events"));
  data->SetInternalField(kKeyEventsCount, Intern(isolate, "_eventsCount"));
  data->SetInternalField(kKeyMaxListeners, Intern(isolate, "_maxListeners"));
  data->SetInternalField(kKeyListener, Intern(isolate, "listener"));
  data->SetInternalField(kKeyError, Intern(isolate, "error"));
  data->SetInternalField(kKeyNewListener, Intern(isolate, "newListener"));
  data->SetInternalField(kKeyRemoveListener, Intern(isolate, "removeListener"));
  data->SetInternalField(kKeyWarned, Intern(isolate, "warned"));
  data->SetInternalField(kKeyLength, Intern(isolate, "length"));

  v8::Local<v8::FunctionTemplate> ctor = v8::FunctionTemplate::New(
      isolate, Constructor, data, v8::Local<v8::Signature>(), 0);
  ctor->SetClassName(Intern(isolate, "EventEmitter"));

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

  return handle_scope.Escape(ctor->GetFunction(context).ToLocalChecked());
}

}  // namespace gin_helper
