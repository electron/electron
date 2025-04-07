// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_BASE_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_BASE_CONVERTER_H_

#include "base/containers/flat_set.h"
#include "base/process/kill.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/std_converter.h"

namespace gin {

template <>
struct Converter<base::TerminationStatus> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::TerminationStatus& status) {
    switch (status) {
      case base::TERMINATION_STATUS_NORMAL_TERMINATION:
        return gin::ConvertToV8(isolate, "clean-exit");
      case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
        return gin::ConvertToV8(isolate, "abnormal-exit");
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
        return gin::ConvertToV8(isolate, "killed");
      case base::TERMINATION_STATUS_PROCESS_CRASHED:
        return gin::ConvertToV8(isolate, "crashed");
      case base::TERMINATION_STATUS_STILL_RUNNING:
        return gin::ConvertToV8(isolate, "still-running");
      case base::TERMINATION_STATUS_LAUNCH_FAILED:
        return gin::ConvertToV8(isolate, "launch-failed");
      case base::TERMINATION_STATUS_OOM:
        return gin::ConvertToV8(isolate, "oom");
#if BUILDFLAG(IS_WIN)
      case base::TERMINATION_STATUS_INTEGRITY_FAILURE:
        return gin::ConvertToV8(isolate, "integrity-failure");
#endif
      case base::TERMINATION_STATUS_MAX_ENUM:
        NOTREACHED();
    }
    NOTREACHED();
  }
};

template <typename T>
struct Converter<base::flat_set<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::flat_set<T>& set) {
    return Converter<std::span<T>>::ToV8(isolate, std::span{set});
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_BASE_CONVERTER_H_
