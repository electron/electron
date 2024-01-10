// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_HID_DEVICE_INFO_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_HID_DEVICE_INFO_CONVERTER_H_

#include "gin/converter.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/browser/hid/hid_chooser_controller.h"
#include "shell/common/gin_converters/value_converter.h"

namespace gin {

template <>
struct Converter<device::mojom::HidDeviceInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const device::mojom::HidDeviceInfoPtr& device) {
    base::Value value = electron::HidChooserContext::DeviceInfoToValue(*device);
    base::Value::Dict& dict = value.GetDict();
    dict.Set("deviceId",
             electron::HidChooserController::PhysicalDeviceIdFromDeviceInfo(
                 *device));
    return gin::Converter<base::Value::Dict>::ToV8(isolate, dict);
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_HID_DEVICE_INFO_CONVERTER_H_
