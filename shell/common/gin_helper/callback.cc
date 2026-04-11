// Copyright (c) 2019 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/callback.h"

#include "content/public/browser/browser_thread.h"
#include "gin/arguments.h"
#include "shell/common/process_util.h"

namespace gin_helper {

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

// Destroy the class on UI thread when possible.
struct DeleteOnUIThread {
  template <typename T>
  static void Destruct(const T* x) {
    if (electron::IsBrowserProcess() &&
        !content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
      content::GetUIThreadTaskRunner({})->DeleteSoon(FROM_HERE, x);
    } else {
      delete x;
    }
  }
};

// Like v8::Global, but ref-counted.
template <typename T>
class RefCountedGlobal
    : public base::RefCountedThreadSafe<RefCountedGlobal<T>, DeleteOnUIThread> {
 public:
  RefCountedGlobal(v8::Isolate* isolate, v8::Local<v8::Value> value)
      : handle_(isolate, value.As<T>()) {}

  [[nodiscard]] bool IsAlive() const { return !handle_.IsEmpty(); }

  v8::Local<T> NewHandle(v8::Isolate* isolate) const {
    return v8::Local<T>::New(isolate, handle_);
  }

 private:
  v8::Global<T> handle_;
};

SafeV8Function::SafeV8Function(v8::Isolate* isolate, v8::Local<v8::Value> value)
    : v8_function_(new RefCountedGlobal<v8::Function>(isolate, value)) {}

SafeV8Function::SafeV8Function(const SafeV8Function& other) = default;

SafeV8Function::~SafeV8Function() = default;

bool SafeV8Function::IsAlive() const {
  return v8_function_.get() && v8_function_->IsAlive();
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
