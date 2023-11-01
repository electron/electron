// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/time_converter.h"

#include "base/time/time.h"
#include "v8/include/v8-date.h"

namespace gin {

v8::Local<v8::Value> Converter<base::Time>::ToV8(v8::Isolate* isolate,
                                                 const base::Time& val) {
  v8::Local<v8::Value> date;
  if (v8::Date::New(isolate->GetCurrentContext(),
                    val.InMillisecondsFSinceUnixEpoch())
          .ToLocal(&date))
    return date;
  else
    return v8::Null(isolate);
}

}  // namespace gin
