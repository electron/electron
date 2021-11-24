// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/bluetooth/electron_bluetooth_delegate.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/public/cpp/bluetooth_uuid.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/lib/bluetooth_chooser.h"
#include "third_party/blink/public/common/bluetooth/web_bluetooth_device_id.h"
#include "third_party/blink/public/mojom/bluetooth/web_bluetooth.mojom.h"

using blink::WebBluetoothDeviceId;
using content::RenderFrameHost;
using content::WebContents;
using device::BluetoothUUID;

namespace electron {

ElectronBluetoothDelegate::ElectronBluetoothDelegate() = default;

ElectronBluetoothDelegate::~ElectronBluetoothDelegate() = default;

std::unique_ptr<content::BluetoothChooser>
ElectronBluetoothDelegate::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  auto* api_web_contents =
      api::WebContents::From(content::WebContents::FromRenderFrameHost(frame));
  return std::make_unique<BluetoothChooser>(api_web_contents, event_handler);
}

// The following methods are not currently called in Electron.
std::unique_ptr<content::BluetoothScanningPrompt>
ElectronBluetoothDelegate::ShowBluetoothScanningPrompt(
    content::RenderFrameHost* frame,
    const content::BluetoothScanningPrompt::EventHandler& event_handler) {
  NOTIMPLEMENTED();
  return nullptr;
}

void ElectronBluetoothDelegate::ShowDeviceCredentialsPrompt(
    content::RenderFrameHost* frame,
    const std::u16string& device_identifier,
    CredentialsCallback callback) {
  // TODO(jkleinsc) implement this
  std::move(callback).Run(DeviceCredentialsPromptResult::kCancelled, u"");
}

WebBluetoothDeviceId ElectronBluetoothDelegate::GetWebBluetoothDeviceId(
    RenderFrameHost* frame,
    const std::string& device_address) {
  NOTIMPLEMENTED();
  return WebBluetoothDeviceId::Create();
}

std::string ElectronBluetoothDelegate::GetDeviceAddress(
    RenderFrameHost* frame,
    const WebBluetoothDeviceId& device_id) {
  NOTIMPLEMENTED();
  return nullptr;
}

WebBluetoothDeviceId ElectronBluetoothDelegate::AddScannedDevice(
    RenderFrameHost* frame,
    const std::string& device_address) {
  NOTIMPLEMENTED();
  return WebBluetoothDeviceId::Create();
}

WebBluetoothDeviceId ElectronBluetoothDelegate::GrantServiceAccessPermission(
    RenderFrameHost* frame,
    const device::BluetoothDevice* device,
    const blink::mojom::WebBluetoothRequestDeviceOptions* options) {
  NOTIMPLEMENTED();
  return WebBluetoothDeviceId::Create();
}

bool ElectronBluetoothDelegate::HasDevicePermission(
    RenderFrameHost* frame,
    const WebBluetoothDeviceId& device_id) {
  NOTIMPLEMENTED();
  return true;
}

bool ElectronBluetoothDelegate::IsAllowedToAccessService(
    RenderFrameHost* frame,
    const WebBluetoothDeviceId& device_id,
    const BluetoothUUID& service) {
  NOTIMPLEMENTED();
  return true;
}

bool ElectronBluetoothDelegate::IsAllowedToAccessAtLeastOneService(
    RenderFrameHost* frame,
    const WebBluetoothDeviceId& device_id) {
  NOTIMPLEMENTED();
  return true;
}

bool ElectronBluetoothDelegate::IsAllowedToAccessManufacturerData(
    RenderFrameHost* frame,
    const WebBluetoothDeviceId& device_id,
    uint16_t manufacturer_code) {
  NOTIMPLEMENTED();
  return true;
}

void ElectronBluetoothDelegate::AddFramePermissionObserver(
    FramePermissionObserver* observer) {
  NOTIMPLEMENTED();
}

void ElectronBluetoothDelegate::RemoveFramePermissionObserver(
    FramePermissionObserver* observer) {
  NOTIMPLEMENTED();
}

std::vector<blink::mojom::WebBluetoothDevicePtr>
ElectronBluetoothDelegate::GetPermittedDevices(
    content::RenderFrameHost* frame) {
  std::vector<blink::mojom::WebBluetoothDevicePtr> permitted_devices;
  NOTIMPLEMENTED();
  return permitted_devices;
}

}  // namespace electron
