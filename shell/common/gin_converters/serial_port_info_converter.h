// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERIAL_PORT_INFO_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERIAL_PORT_INFO_CONVERTER_H_

#include "gin/converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/mojom/serial/serial.mojom.h"

namespace gin {

template <>
struct Converter<device::mojom::SerialPortInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const device::mojom::SerialPortInfoPtr& port) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("portId", port->token.ToString());
    dict.Set("portName", port->path.BaseName().LossyDisplayName());
    if (port->display_name && !port->display_name->empty())
      dict.Set("displayName", *port->display_name);
    if (port->has_vendor_id)
      dict.Set("vendorId", base::StringPrintf("%u", port->vendor_id));
    if (port->has_product_id)
      dict.Set("productId", base::StringPrintf("%u", port->product_id));
    if (port->serial_number && !port->serial_number->empty())
      dict.Set("serialNumber", *port->serial_number);
#if BUILDFLAG(IS_MAC)
    if (port->usb_driver_name && !port->usb_driver_name->empty())
      dict.Set("usbDriverName", *port->usb_driver_name);
#elif BUILDFLAG(IS_WIN)
    if (!port->device_instance_id.empty())
      dict.Set("deviceInstanceId", port->device_instance_id);
#endif
    return gin::ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_SERIAL_PORT_INFO_CONVERTER_H_
