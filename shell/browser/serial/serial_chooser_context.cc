// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_context.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/device_service.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "shell/browser/web_contents_permission_helper.h"

namespace electron {

constexpr char kPortNameKey[] = "name";
constexpr char kTokenKey[] = "token";

#if defined(OS_WIN)
const char kDeviceInstanceIdKey[] = "device_instance_id";
#else
const char kVendorIdKey[] = "vendor_id";
const char kProductIdKey[] = "product_id";
const char kSerialNumberKey[] = "serial_number";
#if defined(OS_MAC)
const char kUsbDriverKey[] = "usb_driver";
#endif  // defined(OS_MAC)
#endif  // defined(OS_WIN

std::string EncodeToken(const base::UnguessableToken& token) {
  const uint64_t data[2] = {token.GetHighForSerialization(),
                            token.GetLowForSerialization()};
  std::string buffer;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(&data[0]), sizeof(data)),
      &buffer);
  return buffer;
}

base::UnguessableToken DecodeToken(base::StringPiece input) {
  std::string buffer;
  if (!base::Base64Decode(input, &buffer) ||
      buffer.length() != sizeof(uint64_t) * 2) {
    return base::UnguessableToken();
  }

  const uint64_t* data = reinterpret_cast<const uint64_t*>(buffer.data());
  return base::UnguessableToken::Deserialize(data[0], data[1]);
}

base::Value PortInfoToValue(const device::mojom::SerialPortInfo& port) {
  base::Value value(base::Value::Type::DICTIONARY);
  if (port.display_name && !port.display_name->empty())
    value.SetStringKey(kPortNameKey, *port.display_name);
  else
    value.SetStringKey(kPortNameKey, port.path.LossyDisplayName());

  if (!SerialChooserContext::CanStorePersistentEntry(port)) {
    value.SetStringKey(kTokenKey, EncodeToken(port.token));
    return value;
  }

#if defined(OS_WIN)
  // Windows provides a handy device identifier which we can rely on to be
  // sufficiently stable for identifying devices across restarts.
  value.SetStringKey(kDeviceInstanceIdKey, port.device_instance_id);
#else
  DCHECK(port.has_vendor_id);
  value.SetIntKey(kVendorIdKey, port.vendor_id);
  DCHECK(port.has_product_id);
  value.SetIntKey(kProductIdKey, port.product_id);
  DCHECK(port.serial_number);
  value.SetStringKey(kSerialNumberKey, *port.serial_number);

#if defined(OS_MAC)
  DCHECK(port.usb_driver_name && !port.usb_driver_name->empty());
  value.SetStringKey(kUsbDriverKey, *port.usb_driver_name);
#endif  // defined(OS_MAC)
#endif  // defined(OS_WIN)
  return value;
}

SerialChooserContext::SerialChooserContext() = default;

SerialChooserContext::~SerialChooserContext() = default;

void SerialChooserContext::GrantPortPermission(
    const url::Origin& origin,
    const device::mojom::SerialPortInfo& port,
    content::RenderFrameHost* render_frame_host) {
  base::Value value = PortInfoToValue(port);
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->GrantSerialPortPermission(origin, std::move(value),
                                               render_frame_host);
}

bool SerialChooserContext::HasPortPermission(
    const url::Origin& origin,
    const device::mojom::SerialPortInfo& port,
    content::RenderFrameHost* render_frame_host) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  base::Value value = PortInfoToValue(port);
  return permission_helper->CheckSerialPortPermission(origin, std::move(value),
                                                      render_frame_host);
}

// static
bool SerialChooserContext::CanStorePersistentEntry(
    const device::mojom::SerialPortInfo& port) {
  // If there is no display name then the path name will be used instead. The
  // path name is not guaranteed to be stable. For example, on Linux the name
  // "ttyUSB0" is reused for any USB serial device. A name like that would be
  // confusing to show in settings when the device is disconnected.
  if (!port.display_name || port.display_name->empty())
    return false;

#if defined(OS_WIN)
  return !port.device_instance_id.empty();
#else
  if (!port.has_vendor_id || !port.has_product_id || !port.serial_number ||
      port.serial_number->empty()) {
    return false;
  }

#if defined(OS_MAC)
  // The combination of the standard USB vendor ID, product ID and serial
  // number properties should be enough to uniquely identify a device
  // however recent versions of macOS include built-in drivers for common
  // types of USB-to-serial adapters while their manufacturers still
  // recommend installing their custom drivers. When both are loaded two
  // IOSerialBSDClient instances are found for each device. Including the
  // USB driver name allows us to distinguish between the two.
  if (!port.usb_driver_name || port.usb_driver_name->empty())
    return false;
#endif  // defined(OS_MAC)

  return true;
#endif  // defined(OS_WIN)
}

device::mojom::SerialPortManager* SerialChooserContext::GetPortManager() {
  EnsurePortManagerConnection();
  return port_manager_.get();
}

void SerialChooserContext::AddPortObserver(PortObserver* observer) {
  port_observer_list_.AddObserver(observer);
}

void SerialChooserContext::RemovePortObserver(PortObserver* observer) {
  port_observer_list_.RemoveObserver(observer);
}

base::WeakPtr<SerialChooserContext> SerialChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SerialChooserContext::OnPortAdded(device::mojom::SerialPortInfoPtr port) {
  for (auto& observer : port_observer_list_)
    observer.OnPortAdded(*port);
}

void SerialChooserContext::OnPortRemoved(
    device::mojom::SerialPortInfoPtr port) {
  for (auto& observer : port_observer_list_)
    observer.OnPortRemoved(*port);
}

void SerialChooserContext::EnsurePortManagerConnection() {
  if (port_manager_)
    return;

  mojo::PendingRemote<device::mojom::SerialPortManager> manager;
  content::GetDeviceService().BindSerialPortManager(
      manager.InitWithNewPipeAndPassReceiver());
  SetUpPortManagerConnection(std::move(manager));
}

void SerialChooserContext::SetUpPortManagerConnection(
    mojo::PendingRemote<device::mojom::SerialPortManager> manager) {
  port_manager_.Bind(std::move(manager));
  port_manager_.set_disconnect_handler(
      base::BindOnce(&SerialChooserContext::OnPortManagerConnectionError,
                     base::Unretained(this)));

  port_manager_->SetClient(client_receiver_.BindNewPipeAndPassRemote());
}

void SerialChooserContext::OnPortManagerConnectionError() {
  port_manager_.reset();
  client_receiver_.reset();
}

}  // namespace electron
