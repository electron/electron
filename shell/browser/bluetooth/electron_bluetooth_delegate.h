// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_BLUETOOTH_ELECTRON_BLUETOOTH_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_BLUETOOTH_ELECTRON_BLUETOOTH_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "content/public/browser/bluetooth_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "third_party/blink/public/mojom/bluetooth/web_bluetooth.mojom-forward.h"

namespace blink {
class WebBluetoothDeviceId;
}

namespace content {
class RenderFrameHost;
}

namespace device {
class BluetoothDevice;
class BluetoothUUID;
}  // namespace device

namespace electron {

// Provides an interface for managing device permissions for Web Bluetooth and
// Web Bluetooth Scanning API. This is the Electron-specific implementation of
// the BluetoothDelegate.
class ElectronBluetoothDelegate : public content::BluetoothDelegate {
 public:
  ElectronBluetoothDelegate();
  ~ElectronBluetoothDelegate() override;

  // Move-only class.
  ElectronBluetoothDelegate(const ElectronBluetoothDelegate&) = delete;
  ElectronBluetoothDelegate& operator=(const ElectronBluetoothDelegate&) =
      delete;

  // BluetoothDelegate implementation:
  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& event_handler) override;

  std::unique_ptr<content::BluetoothScanningPrompt> ShowBluetoothScanningPrompt(
      content::RenderFrameHost* frame,
      const content::BluetoothScanningPrompt::EventHandler& event_handler)
      override;
  void ShowDevicePairPrompt(content::RenderFrameHost* frame,
                            const std::u16string& device_identifier,
                            PairPromptCallback callback,
                            PairingKind pairing_kind,
                            const absl::optional<std::u16string>& pin) override;
  blink::WebBluetoothDeviceId GetWebBluetoothDeviceId(
      content::RenderFrameHost* frame,
      const std::string& device_address) override;
  std::string GetDeviceAddress(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  blink::WebBluetoothDeviceId AddScannedDevice(
      content::RenderFrameHost* frame,
      const std::string& device_address) override;
  blink::WebBluetoothDeviceId GrantServiceAccessPermission(
      content::RenderFrameHost* frame,
      const device::BluetoothDevice* device,
      const blink::mojom::WebBluetoothRequestDeviceOptions* options) override;
  bool HasDevicePermission(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  void RevokeDevicePermissionWebInitiated(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  bool IsAllowedToAccessService(content::RenderFrameHost* frame,
                                const blink::WebBluetoothDeviceId& device_id,
                                const device::BluetoothUUID& service) override;
  bool IsAllowedToAccessAtLeastOneService(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  bool IsAllowedToAccessManufacturerData(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id,
      uint16_t manufacturer_code) override;
  std::vector<blink::mojom::WebBluetoothDevicePtr> GetPermittedDevices(
      content::RenderFrameHost* frame) override;
  void AddFramePermissionObserver(FramePermissionObserver* observer) override;
  void RemoveFramePermissionObserver(
      FramePermissionObserver* observer) override;

 private:
  void OnDevicePairPromptResponse(PairPromptCallback callback,
                                  base::Value::Dict response);

  base::WeakPtrFactory<ElectronBluetoothDelegate> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_BLUETOOTH_ELECTRON_BLUETOOTH_DELEGATE_H_
