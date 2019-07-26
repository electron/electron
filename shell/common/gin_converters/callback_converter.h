// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_

#include "gin/converter.h"
#include "gin/function_template.h"

// TODO(deermichel): check if this is a good idea?!
// ref:
// https://github.com/electron/electron/blob/master/shell/common/native_mate_converters/callback.h#L149

namespace gin {

template <typename Sig>
struct Converter<base::RepeatingCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::RepeatingCallback<Sig>& val) {
    return gin::CreateFunctionTemplate(isolate, val)
        ->GetFunction(isolate->GetCurrentContext())
        .ToLocalChecked();
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_
