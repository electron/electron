// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTOR_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTOR_H_

#include <tuple>

#include "shell/common/gin_helper/function_template.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace gin_helper {

namespace internal {

// Convert a `gin::Argument`'s arguments into a tuple of native types
// by iteratively calling gin_helper::GetNextArgument().
template <typename... Types>
class GinArgumentsToTuple {
 public:
  [[nodiscard]] static std::pair<bool /*ok*/, std::tuple<Types...>> GetArgs(
      gin::Arguments* args) {
    bool ok = true;
    InvokerOptions opts{.holder_is_first_argument = true};
    auto tup = std::make_tuple(GetNextArg<Types>(args, opts, ok)...);
    return {ok, std::move(tup)};
  }

 private:
  template <typename T>
  static T GetNextArg(gin::Arguments* args, InvokerOptions& opts, bool& ok) {
    auto val = T{};
    ok = ok && gin_helper::GetNextArgument(args, opts, 0, &val);
    opts.holder_is_first_argument = false;
    return val;
  }
};

// Invoke a callback with arguments extracted from `args`.
template <typename... Types>
WrappableBase* InvokeFactory(
    gin::Arguments* const args,
    const base::RepeatingCallback<WrappableBase*(Types...)>& callback) {
  auto [ok, tup] = GinArgumentsToTuple<Types...>::GetArgs(args);
  if (!ok)
    return {};
  return std::apply(
      [&callback](Types... args) { return callback.Run(std::move(args)...); },
      std::move(tup));
}

template <typename Sig>
void InvokeNew(const base::RepeatingCallback<Sig>& factory,
               v8::Isolate* const isolate,
               gin::Arguments* const args) {
  if (!args->IsConstructCall()) {
    args->ThrowTypeError("Requires constructor call");
    return;
  }

  WrappableBase* object;
  {
    // Don't continue if the constructor throws an exception.
    v8::TryCatch try_catch{isolate};
    object = internal::InvokeFactory(args, factory);
    if (try_catch.HasCaught()) {
      try_catch.ReThrow();
      return;
    }
  }

  if (!object)
    args->ThrowError();

  return;
}

}  // namespace internal

// Create a FunctionTemplate that can be "new"ed in JavaScript.
// It is user's responsibility to ensure this function is called for one type
// only ONCE in the program's whole lifetime, otherwise we would have memory
// leak.
template <typename T, typename Sig>
v8::Local<v8::Function> CreateConstructor(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig>& func) {
#ifndef NDEBUG
  static bool called = false;
  CHECK(!called) << "CreateConstructor can only be called for one type once";
  called = true;
#endif
  v8::Local<v8::FunctionTemplate> templ = gin_helper::CreateFunctionTemplate(
      isolate, base::BindRepeating(&internal::InvokeNew<Sig>, func));
  templ->InstanceTemplate()->SetInternalFieldCount(1);
  T::BuildPrototype(isolate, templ);
  return templ->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
}

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTOR_H_
