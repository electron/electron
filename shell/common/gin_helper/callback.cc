// Copyright (c) 2019 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/callback.h"

#include "content/public/browser/browser_thread.h"
#include "gin/dictionary.h"
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

  static gin::DeprecatedWrapperInfo kWrapperInfo;

  v8::Global<v8::External> handle;
  Translator translator;
};

gin::DeprecatedWrapperInfo TranslatorHolder::kWrapperInfo = {
    gin::kEmbedderNativeGin};

void CallTranslator(v8::Local<v8::External> external,
                    v8::Local<v8::Object> state,
                    gin::Arguments* args) {
  // Whether the callback should only be called once.
  v8::Isolate* isolate = args->isolate();
  auto context = isolate->GetCurrentContext();
  bool one_time =
      state->Has(context, gin::StringToSymbol(isolate, "oneTime")).ToChecked();

  // Check if the callback has already been called.
  if (one_time) {
    auto called_symbol = gin::StringToSymbol(isolate, "called");
    if (state->Has(context, called_symbol).ToChecked()) {
      args->ThrowTypeError("One-time callback was called more than once");
      return;
    } else {
      state->Set(context, called_symbol, v8::True(isolate)).ToChecked();
    }
  }

  auto* holder = static_cast<TranslatorHolder*>(
      external->Value(v8::kExternalPointerTypeTagDefault));
  holder->translator.Run(args);

  // Free immediately for one-time callback.
  if (one_time)
    delete holder;
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
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  auto* wrapper_info = &TranslatorHolder::kWrapperInfo;
  v8::Local<v8::FunctionTemplate> constructor =
      data->DeprecatedGetFunctionTemplate(wrapper_info);
  // The FunctionTemplate is cached.
  if (constructor.IsEmpty()) {
    constructor =
        CreateFunctionTemplate(isolate, base::BindRepeating(&CallTranslator));
    data->DeprecatedSetFunctionTemplate(wrapper_info, constructor);
  }

  auto* holder = new TranslatorHolder(isolate);
  holder->translator = translator;
  auto state = gin::Dictionary::CreateEmpty(isolate);
  if (one_time)
    state.Set("oneTime", true);
  auto context = isolate->GetCurrentContext();
  return BindFunctionWith(
      isolate, context, constructor->GetFunction(context).ToLocalChecked(),
      holder->handle.Get(isolate), gin::ConvertToV8(isolate, state));
}

// func.bind(func, arg1).
// NB(zcbenz): Using C++11 version crashes VS.
v8::Local<v8::Value> BindFunctionWith(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      v8::Local<v8::Function> func,
                                      v8::Local<v8::Value> arg1,
                                      v8::Local<v8::Value> arg2) {
  v8::MaybeLocal<v8::Value> bind =
      func->Get(context, gin::StringToV8(isolate, "bind"));
  CHECK(!bind.IsEmpty());
  v8::Local<v8::Function> bind_func = bind.ToLocalChecked().As<v8::Function>();
  v8::Local<v8::Value> converted[] = {func, arg1, arg2};
  return bind_func->Call(context, func, std::size(converted), converted)
      .ToLocalChecked();
}

}  // namespace gin_helper
