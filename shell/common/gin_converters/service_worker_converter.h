// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERVICE_WORKER_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERVICE_WORKER_CONVERTER_H_

#include "gin/converter.h"
#include "third_party/blink/public/common/service_worker/embedded_worker_status.h"

namespace gin {

template <>
struct Converter<blink::EmbeddedWorkerStatus> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::EmbeddedWorkerStatus& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERVICE_WORKER_CONVERTER_H_
