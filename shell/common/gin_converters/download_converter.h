/*** OpenFin ***/
// Copyright 2021 OpenFin. All rights reserved.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_DOWNLOAD_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_DOWNLOAD_CONVERTER_H_

#include "components/download/public/common/download_interrupt_reasons.h"
#include "gin/converter.h"

#include "electron/shell/common/gin_converters/std_converter.h"

namespace gin {

template <>
struct Converter<download::DownloadInterruptReason> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const download::DownloadInterruptReason& val) {
    const auto string_val = download::DownloadInterruptReasonToString(val);
    return Converter<std::string>::ToV8(isolate, string_val);
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_DOWNLOAD_CONVERTER_H_
