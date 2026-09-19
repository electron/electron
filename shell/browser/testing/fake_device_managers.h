// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_TESTING_FAKE_DEVICE_MANAGERS_H_
#define ELECTRON_SHELL_BROWSER_TESTING_FAKE_DEVICE_MANAGERS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "services/device/public/mojom/serial.mojom.h"
#include "services/device/public/mojom/usb_device.mojom.h"
#include "services/device/public/mojom/usb_manager.mojom.h"
#include "services/device/public/mojom/usb_manager_client.mojom.h"

namespace electron {

class ElectronBrowserContext;

// In-process stand-ins for the device service's HID, USB and serial managers,
// used by the spec runner (process._linkedBinding('electron_common_testing'))
// to exercise the WebHID / WebUSB / Web Serial permission paths without
// hardware. Devices can be opened; I/O is not implemented.
class FakeHidManager : public device::mojom::HidManager {
 public:
  FakeHidManager();
  ~FakeHidManager() override;

  mojo::PendingRemote<device::mojom::HidManager> Bind();
  // Returns the new device's guid.
  std::string AddDevice(uint16_t vendor_id,
                        uint16_t product_id,
                        const std::string& product_name,
                        const std::string& serial_number);
  void RemoveDevice(const std::string& guid);
  size_t OpenConnectionCount();

  // device::mojom::HidManager:
  void GetDevicesAndSetClient(
      mojo::PendingAssociatedRemote<device::mojom::HidManagerClient> client,
      GetDevicesAndSetClientCallback callback) override;
  void GetDevices(GetDevicesCallback callback) override;
  void Connect(
      const std::string& device_guid,
      mojo::PendingRemote<device::mojom::HidConnectionClient> connection_client,
      mojo::PendingRemote<device::mojom::HidConnectionWatcher> watcher,
      bool allow_protected_reports,
      bool allow_fido_reports,
      ConnectCallback callback) override;
  void AddReceiver(
      mojo::PendingReceiver<device::mojom::HidManager> receiver) override;

 private:
  class Connection;
  std::map<std::string, device::mojom::HidDeviceInfoPtr> devices_;
  std::vector<std::unique_ptr<Connection>> connections_;
  mojo::AssociatedRemoteSet<device::mojom::HidManagerClient> clients_;
  mojo::ReceiverSet<device::mojom::HidManager> receivers_;
};

class FakeUsbDeviceManager : public device::mojom::UsbDeviceManager {
 public:
  FakeUsbDeviceManager();
  ~FakeUsbDeviceManager() override;

  mojo::PendingRemote<device::mojom::UsbDeviceManager> Bind();
  std::string AddDevice(uint16_t vendor_id,
                        uint16_t product_id,
                        const std::string& product_name,
                        const std::string& serial_number);
  void RemoveDevice(const std::string& guid);
  size_t OpenConnectionCount();

  // device::mojom::UsbDeviceManager:
  void EnumerateDevicesAndSetClient(
      mojo::PendingAssociatedRemote<device::mojom::UsbDeviceManagerClient>
          client,
      EnumerateDevicesAndSetClientCallback callback) override;
  void GetDevices(device::mojom::UsbEnumerationOptionsPtr options,
                  GetDevicesCallback callback) override;
  void GetDevice(
      const std::string& guid,
      const std::vector<uint8_t>& blocked_interface_classes,
      mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
      mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client)
      override;
  void GetUnrestrictedDevice(
      const std::string& guid,
      const std::vector<uint8_t>& blocked_interface_classes,
      mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
      mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client)
      override;
  void GetSecurityKeyDevice(
      const std::string& guid,
      mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
      mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client)
      override;
  void SetClient(
      mojo::PendingAssociatedRemote<device::mojom::UsbDeviceManagerClient>
          client) override;

 private:
  class Device;
  std::map<std::string, device::mojom::UsbDeviceInfoPtr> devices_;
  std::vector<std::unique_ptr<Device>> open_devices_;
  mojo::AssociatedRemoteSet<device::mojom::UsbDeviceManagerClient> clients_;
  mojo::ReceiverSet<device::mojom::UsbDeviceManager> receivers_;
};

class FakeSerialPortManager : public device::mojom::SerialPortManager {
 public:
  FakeSerialPortManager();
  ~FakeSerialPortManager() override;

  mojo::PendingRemote<device::mojom::SerialPortManager> Bind();
  // Returns the token as a string.
  std::string AddPort(const std::string& path,
                      const std::string& display_name,
                      uint16_t vendor_id,
                      uint16_t product_id,
                      const std::string& serial_number);
  void RemovePort(const std::string& token);
  void SetPortConnected(const std::string& token, bool connected);
  size_t OpenConnectionCount();

  // device::mojom::SerialPortManager:
  void SetClient(mojo::PendingRemote<device::mojom::SerialPortManagerClient>
                     client) override;
  void GetDevices(bool allow_bluetooth_system_prompt,
                  GetDevicesCallback callback) override;
  void OpenPort(
      const base::UnguessableToken& token,
      bool use_alternate_path,
      device::mojom::SerialConnectionOptionsPtr options,
      mojo::PendingRemote<device::mojom::SerialPortClient> client,
      mojo::PendingRemote<device::mojom::SerialPortConnectionWatcher> watcher,
      OpenPortCallback callback) override;

 private:
  class Port;
  std::map<base::UnguessableToken, device::mojom::SerialPortInfoPtr> ports_;
  std::vector<std::unique_ptr<Port>> open_ports_;
  mojo::RemoteSet<device::mojom::SerialPortManagerClient> clients_;
  mojo::ReceiverSet<device::mojom::SerialPortManager> receivers_;
};

// Owns one set of fakes per browser context and wires them into that
// context's chooser contexts.
class FakeDeviceManagers {
 public:
  static FakeDeviceManagers* GetOrCreate(ElectronBrowserContext* context);
  static FakeDeviceManagers* Get(ElectronBrowserContext* context);

  FakeHidManager& hid() { return hid_; }
  FakeUsbDeviceManager& usb() { return usb_; }
  FakeSerialPortManager& serial() { return serial_; }

 private:
  FakeDeviceManagers() = default;

  FakeHidManager hid_;
  FakeUsbDeviceManager usb_;
  FakeSerialPortManager serial_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_TESTING_FAKE_DEVICE_MANAGERS_H_
