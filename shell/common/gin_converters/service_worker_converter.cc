// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/service_worker_converter.h"

#include "base/containers/fixed_flat_map.h"

namespace gin {

// static
v8::Local<v8::Value> Converter<blink::EmbeddedWorkerStatus>::ToV8(
    v8::Isolate* isolate,
    const blink::EmbeddedWorkerStatus& val) {
  static constexpr auto Lookup =
      base::MakeFixedFlatMap<blink::EmbeddedWorkerStatus, std::string_view>({
          {blink::EmbeddedWorkerStatus::kStarting, "starting"},
          {blink::EmbeddedWorkerStatus::kRunning, "running"},
          {blink::EmbeddedWorkerStatus::kStopping, "stopping"},
          {blink::EmbeddedWorkerStatus::kStopped, "stopped"},
      });
  return StringToV8(isolate, Lookup.at(val));
}

}  // namespace gin
