// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_context.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "chrome/browser/serial/serial_blocklist.h"
#include "content/public/browser/device_service.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_permission_manager.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/serial_port_info_converter.h"

namespace electron {

namespace {

std::string EncodeToken(const base::UnguessableToken& token) {
  const uint64_t data[2] = {token.GetHighForSerialization(),
                            token.GetLowForSerialization()};
  return base::Base64Encode(base::as_byte_span(data));
}

base::Value PortInfoToValue(const device::mojom::SerialPortInfo& port) {
  base::DictValue value;
  if (port.display_name && !port.display_name->empty()) {
    value.Set(kPortNameKey, *port.display_name);
  } else {
    value.Set(kPortNameKey, port.path.LossyDisplayName());
  }

  if (!SerialChooserContext::CanStorePersistentEntry(port)) {
    value.Set(kTokenKey, EncodeToken(port.token));
    return base::Value(std::move(value));
  }

  if (port.bluetooth_service_class_id &&
      port.bluetooth_service_class_id->IsValid()) {
    value.Set(kBluetoothDevicePathKey, port.path.LossyDisplayName());
  } else {
#if BUILDFLAG(IS_WIN)
    // Windows provides a handy device identifier which we can rely on to be
    // sufficiently stable for identifying devices across restarts.
    value.Set(kDeviceInstanceIdKey, port.device_instance_id);
#else
    CHECK(port.has_vendor_id);
    value.Set(kVendorIdKey, port.vendor_id);
    CHECK(port.has_product_id);
    value.Set(kProductIdKey, port.product_id);
    CHECK(port.serial_number);
    value.Set(kSerialNumberKey, *port.serial_number);
#if BUILDFLAG(IS_MAC)
    CHECK(port.usb_driver_name && !port.usb_driver_name->empty());
    value.Set(kUsbDriverKey, *port.usb_driver_name);
#endif  // BUILDFLAG(IS_MAC)
#endif  // BUILDFLAG(IS_WIN)
  }
  return base::Value(std::move(value));
}

}  // namespace

SerialChooserContext::SerialChooserContext(ElectronBrowserContext* context)
    : browser_context_(context) {}

SerialChooserContext::~SerialChooserContext() {
  // Notify observers that the chooser context is about to be destroyed.
  // Observers must remove themselves from the observer lists.
  for (auto& observer : port_observer_list_) {
    observer.OnSerialChooserContextShutdown();
    DCHECK(!port_observer_list_.HasObserver(&observer));
  }
}

void SerialChooserContext::GrantPortPermission(
    const url::Origin& origin,
    const device::mojom::SerialPortInfo& port,
    content::RenderFrameHost* render_frame_host) {
  port_info_.try_emplace(port.token, port.Clone());

  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  // Remember the selection for this session in every case; it is what makes
  // requestPort() -> open() work without a persistent identifier and what a
  // device permission handler sees as details.selected.
  ephemeral_ports_[origin].insert(port.token);
  if (CanStorePersistentEntry(port) &&
      !permission_manager->HasDevicePermissionHandler()) {
    permission_manager->GrantDevicePermission(blink::PermissionType::SERIAL,
                                              origin, PortInfoToValue(port),
                                              browser_context_);
  }
}

bool SerialChooserContext::HasPortPermission(
    const url::Origin& origin,
    const device::mojom::SerialPortInfo& port,
    content::RenderFrameHost* render_frame_host) {
  bool blocklist_disabled = base::CommandLine::ForCurrentProcess()->HasSwitch(
      kDisableSerialBlocklist);
  if (!blocklist_disabled && SerialBlocklist::Get().IsExcluded(port)) {
    return false;
  }

  auto it = ephemeral_ports_.find(origin);
  const bool selected =
      it != ephemeral_ports_.end() && it->second.contains(port.token);

  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  base::Value value = PortInfoToValue(port);
  // The id select-serial-port reported for this port, so a handler can match a
  // selection it recorded.
  value.GetDict().Set("portId", port.token.ToString());
  return permission_manager->CheckDevicePermission(
      blink::PermissionType::SERIAL, origin, value, browser_context_,
      render_frame_host, selected);
}

void SerialChooserContext::RevokePortPermissionWebInitiated(
    const url::Origin& requesting_origin,
    const base::UnguessableToken& token,
    content::RenderFrameHost* render_frame_host) {
  // |requesting_origin| is owned by the frame the JS below can destroy.
  const url::Origin origin = requesting_origin;
  auto ephemeral = ephemeral_ports_.find(origin);
  if (ephemeral != ephemeral_ports_.end()) {
    ephemeral->second.erase(token);
    if (ephemeral->second.empty())
      ephemeral_ports_.erase(ephemeral);
  }

  auto it = port_info_.find(token);
  if (it == port_info_.end())
    return;
  // Keep a copy: the JS below may remove the port.
  device::mojom::SerialPortInfoPtr port = it->second->Clone();

  auto* permission_manager = static_cast<ElectronPermissionManager*>(
      browser_context_->GetPermissionControllerDelegate());
  permission_manager->RevokeDevicePermission(blink::PermissionType::SERIAL,
                                             origin, PortInfoToValue(*port),
                                             browser_context_);

  gin::WeakCell<api::Session>* session =
      api::Session::FromBrowserContext(browser_context_);
  if (session && session->Get()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    auto details = gin_helper::Dictionary::CreateEmpty(isolate);
    details.Set("port", port);
    details.SetGetter("frame", render_frame_host);
    details.Set("origin", origin.Serialize());
    session->Get()->Emit("serial-port-revoked", details);
  }
  // Let every SerialService for this origin drop connections to ports it no
  // longer has permission for (content re-checks HasPortPermission(), which may
  // run app JS, so do it from a fresh task rather than under the caller).
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&SerialChooserContext::NotifyPermissionRevoked,
                                weak_factory_.GetWeakPtr(), origin));
}

void SerialChooserContext::NotifyPermissionRevoked(const url::Origin& origin) {
  for (auto& observer : port_observer_list_)
    observer.OnPermissionRevoked(origin);
}

void SerialChooserContext::OnPortConnectedStateChanged(
    device::mojom::SerialPortInfoPtr port) {
  // Bluetooth serial ports stay enumerated and only toggle their connected
  // state; keep the cached info current and tell SerialService so that
  // navigator.serial fires connect/disconnect.
  auto it = port_info_.find(port->token);
  if (it != port_info_.end())
    it->second->connected = port->connected;
  for (auto& observer : port_observer_list_)
    observer.OnPortConnectedStateChanged(*port);
}

void SerialChooserContext::SetPortManagerForTesting(
    mojo::PendingRemote<device::mojom::SerialPortManager> manager) {
  OnPortManagerConnectionError();
  SetUpPortManagerConnection(std::move(manager));
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

  const bool has_bluetooth = port.bluetooth_service_class_id &&
                             port.bluetooth_service_class_id->IsValid() &&
                             !port.path.empty();
  if (has_bluetooth) {
    return true;
  }

#if BUILDFLAG(IS_WIN)
  return !port.device_instance_id.empty();
#else
  const bool has_usb = port.has_vendor_id && port.has_product_id &&
                       port.serial_number && !port.serial_number->empty();
  if (!has_usb) {
    return false;
  }

#if BUILDFLAG(IS_MAC)
  // The combination of the standard USB vendor ID, product ID and serial
  // number properties should be enough to uniquely identify a device
  // however recent versions of macOS include built-in drivers for common
  // types of USB-to-serial adapters while their manufacturers still
  // recommend installing their custom drivers. When both are loaded two
  // IOSerialBSDClient instances are found for each device. Including the
  // USB driver name allows us to distinguish between the two.
  if (!port.usb_driver_name || port.usb_driver_name->empty())
    return false;
#endif  // BUILDFLAG(IS_MAC)

  return true;
#endif  // BUILDFLAG(IS_WIN)
}

const device::mojom::SerialPortInfo* SerialChooserContext::GetPortInfo(
    const base::UnguessableToken& token) {
  DCHECK(is_initialized_);
  auto it = port_info_.find(token);
  return it == port_info_.end() ? nullptr : it->second.get();
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
  if (!port_info_.contains(port->token))
    port_info_.insert({port->token, port->Clone()});

  for (auto& map_entry : ephemeral_ports_) {
    std::set<base::UnguessableToken>& ports = map_entry.second;
    ports.erase(port->token);
  }

  port_observer_list_.Notify(&PortObserver::OnPortAdded, *port);
}

void SerialChooserContext::OnPortRemoved(
    device::mojom::SerialPortInfoPtr port) {
  port_observer_list_.Notify(&PortObserver::OnPortRemoved, *port);
  port_info_.erase(port->token);
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
  // Pass false for `allow_bluetooth_system_prompt` to avoid triggering the
  // macOS Bluetooth permission prompt during background initialization. Any
  // explicit user request to find devices (e.g. via `requestPort`) will use
  // true for the `GetDevices` call.
  port_manager_->GetDevices(/*allow_bluetooth_system_prompt=*/false,
                            base::BindOnce(&SerialChooserContext::OnGetDevices,
                                           weak_factory_.GetWeakPtr()));
}

void SerialChooserContext::OnGetDevices(
    std::vector<device::mojom::SerialPortInfoPtr> ports) {
  for (auto& port : ports)
    port_info_.try_emplace(port->token, std::move(port));
  is_initialized_ = true;
}

void SerialChooserContext::OnPortManagerConnectionError() {
  port_manager_.reset();
  client_receiver_.reset();

  port_info_.clear();
  ephemeral_ports_.clear();
}
}  // namespace electron
