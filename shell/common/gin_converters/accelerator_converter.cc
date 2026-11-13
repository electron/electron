// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/accelerator_converter.h"

#include <string>

#include "shell/browser/ui/accelerator_util.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace gin {

namespace {

bool IsNullOrUndefined(v8::Local<v8::Value> value) {
  return value->IsNull() || value->IsUndefined();
}

}  // namespace

// static
bool Converter<ui::Accelerator>::FromV8(v8::Isolate* isolate,
                                        v8::Local<v8::Value> val,
                                        ui::Accelerator* out) {
  std::string keycode;
  if (!ConvertFromV8(isolate, val, &keycode))
    return false;
  return accelerator_util::StringToAccelerator(keycode, out);
}

// static
bool Converter<electron::ShortcutRegistration>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    electron::ShortcutRegistration* out) {
  if (val->IsString()) {
    out->accelerator = V8ToString(isolate, val);
    out->description.clear();
    return true;
  }

  if (!val->IsObject() || val->IsArray()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Expected a string or an object with an 'accelerator' property");
    return false;
  }

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> object = val.As<v8::Object>();

  v8::Local<v8::Value> accelerator;
  if (!object->Get(context, StringToV8(isolate, "accelerator"))
           .ToLocal(&accelerator) ||
      IsNullOrUndefined(accelerator)) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "The 'accelerator' property is required");
    return false;
  }
  if (!accelerator->IsString()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "The 'accelerator' property must be a string");
    return false;
  }
  out->accelerator = V8ToString(isolate, accelerator);

  v8::Local<v8::Value> description;
  if (object->Get(context, StringToV8(isolate, "description"))
          .ToLocal(&description) &&
      !IsNullOrUndefined(description)) {
    if (!description->IsString()) {
      gin_helper::ErrorThrower(isolate).ThrowTypeError(
          "The 'description' property must be a string");
      return false;
    }
    if (!ConvertFromV8(isolate, description, &out->description))
      return false;
  }

  return true;
}

}  // namespace gin
