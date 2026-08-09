// Copyright (c) 2019 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/callback.h"

#include "gin/arguments.h"
#include "gin/persistent.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-traced-handle.h"

namespace gin_helper {

class SafeV8FunctionHandle final
    : public cppgc::GarbageCollected<SafeV8FunctionHandle> {
 public:
  SafeV8FunctionHandle(v8::Isolate* isolate, v8::Local<v8::Value> value)
      : v8_function_(isolate, value.As<v8::Function>()) {}

  void Trace(cppgc::Visitor* visitor) const { visitor->Trace(v8_function_); }

  [[nodiscard]] bool IsAlive() const { return !v8_function_.IsEmpty(); }

  v8::Local<v8::Function> NewHandle(v8::Isolate* isolate) const {
    return v8_function_.Get(isolate);
  }

 private:
  v8::TracedReference<v8::Function> v8_function_;
};

namespace {

struct TranslatorHolder {
  explicit TranslatorHolder(v8::Isolate* isolate)
      : handle(isolate,
               v8::External::New(isolate,
                                 this,
                                 v8::kExternalPointerTypeTagDefault)) {
    handle.SetWeak(this, &GC, v8::WeakCallbackType::kParameter);
  }
  ~TranslatorHolder() {
    if (!handle.IsEmpty()) {
      handle.ClearWeak();
      handle.Reset();
    }
  }

  static void GC(const v8::WeakCallbackInfo<TranslatorHolder>& data) {
    delete data.GetParameter();
  }

  v8::Global<v8::External> handle;
  Translator translator;
  bool one_time = false;
  bool called = false;
};

void CallTranslator(const v8::FunctionCallbackInfo<v8::Value>& info) {
  gin::Arguments args(info);
  auto* holder =
      static_cast<TranslatorHolder*>(info.Data().As<v8::External>()->Value(
          v8::kExternalPointerTypeTagDefault));

  if (holder->one_time && holder->called) {
    args.ThrowTypeError("One-time callback was called more than once");
    return;
  }
  holder->called = true;

  holder->translator.Run(&args);

  if (holder->one_time)
    holder->translator.Reset();
}

}  // namespace

SafeV8Function::SafeV8Function(v8::Isolate* isolate, v8::Local<v8::Value> value)
    : v8_function_(
          gin::WrapPersistent(cppgc::MakeGarbageCollected<SafeV8FunctionHandle>(
              isolate->GetCppHeap()->GetAllocationHandle(),
              isolate,
              value))) {}

SafeV8Function::SafeV8Function(const SafeV8Function& other) = default;

SafeV8Function::~SafeV8Function() = default;

bool SafeV8Function::IsAlive() const {
  return v8_function_ && v8_function_->IsAlive();
}

v8::Local<v8::Function> SafeV8Function::NewHandle(v8::Isolate* isolate) const {
  return v8_function_->NewHandle(isolate);
}

v8::Local<v8::Value> CreateFunctionFromTranslator(v8::Isolate* isolate,
                                                  const Translator& translator,
                                                  bool one_time) {
  auto* holder = new TranslatorHolder(isolate);
  holder->translator = translator;
  holder->one_time = one_time;
  auto context = isolate->GetCurrentContext();
  return v8::Function::New(context, CallTranslator, holder->handle.Get(isolate))
      .ToLocalChecked();
}

}  // namespace gin_helper
