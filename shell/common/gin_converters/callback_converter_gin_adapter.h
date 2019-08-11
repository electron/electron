// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_GIN_ADAPTER_H_

#include "gin/converter.h"
#include "shell/common/native_mate_converters/once_callback.h"

// TODO(zcbenz): Move the implementations from native_mate_converters to here.

namespace gin {

template <typename Sig>
struct Converter<base::RepeatingCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::RepeatingCallback<Sig>& in) {
    return mate::ConvertToV8(isolate, in);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::RepeatingCallback<Sig>* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
};

template <typename Sig>
struct Converter<base::OnceCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   base::OnceCallback<Sig> in) {
    return mate::ConvertToV8(isolate, in);
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::OnceCallback<Sig>* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_GIN_ADAPTER_H_
