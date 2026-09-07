// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/testing/fake_device_managers.h"

#include <utility>

#include "base/containers/map_util.h"
#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "content/browser/bluetooth/bluetooth_adapter_factory_wrapper.h"  // nogncheck
#include "content/browser/bluetooth/bluetooth_device_chooser_controller.h"  // nogncheck
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/emulation/fake_central.h"
#include "device/bluetooth/public/mojom/emulation/fake_bluetooth.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "services/device/public/mojom/serial.mojom.h"
#include "services/device/public/mojom/usb_device.mojom.h"
#include "services/device/public/mojom/usb_enumeration_options.mojom.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/browser/hid/hid_chooser_context_factory.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"
#include "shell/browser/usb/usb_chooser_context.h"
#include "shell/browser/usb/usb_chooser_context_factory.h"

namespace electron {

// ---------------------------------------------------------------- HID -----

// An open HID connection. It closes itself when content drops the watcher,
// which is how HidService enforces a revoked permission; the renderer then
// observes the HidConnection pipe closing.
class FakeHidManager::Connection : public device::mojom::HidConnection {
 public:
  Connection(mojo::PendingReceiver<device::mojom::HidConnection> receiver,
             mojo::PendingRemote<device::mojom::HidConnectionClient> client,
             mojo::PendingRemote<device::mojom::HidConnectionWatcher> watcher)
      : receiver_(this, std::move(receiver)) {
    if (client)
      client_.Bind(std::move(client));
    if (watcher) {
      watcher_.Bind(std::move(watcher));
      watcher_.set_disconnect_handler(
          base::BindOnce(&Connection::Close, base::Unretained(this)));
    }
    receiver_.set_disconnect_handler(
        base::BindOnce(&Connection::Close, base::Unretained(this)));
  }
  ~Connection() override = default;

  bool closed() const { return !receiver_.is_bound(); }

  // device::mojom::HidConnection:
  void Read(ReadCallback callback) override {
    std::move(callback).Run(false, 0, std::nullopt);
  }
  void Write(uint8_t report_id,
             const std::vector<uint8_t>& buffer,
             WriteCallback callback) override {
    std::move(callback).Run(true);
  }
  void GetFeatureReport(uint8_t report_id,
                        GetFeatureReportCallback callback) override {
    std::move(callback).Run(false, std::nullopt);
  }
  void SendFeatureReport(uint8_t report_id,
                         const std::vector<uint8_t>& buffer,
                         SendFeatureReportCallback callback) override {
    std::move(callback).Run(true);
  }

 private:
  void Close() {
    receiver_.reset();
    client_.reset();
    watcher_.reset();
  }

  mojo::Receiver<device::mojom::HidConnection> receiver_;
  mojo::Remote<device::mojom::HidConnectionClient> client_;
  mojo::Remote<device::mojom::HidConnectionWatcher> watcher_;
};

FakeHidManager::FakeHidManager() = default;
FakeHidManager::~FakeHidManager() = default;

mojo::PendingRemote<device::mojom::HidManager> FakeHidManager::Bind() {
  mojo::PendingRemote<device::mojom::HidManager> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

std::string FakeHidManager::AddDevice(uint16_t vendor_id,
                                      uint16_t product_id,
                                      const std::string& product_name,
                                      const std::string& serial_number) {
  auto collection = device::mojom::HidCollectionInfo::New();
  collection->usage = device::mojom::HidUsageAndPage::New(
      /*usage=*/1, device::mojom::kPageVendor);
  collection->collection_type = device::mojom::kHIDCollectionTypeApplication;
  collection->input_reports.push_back(
      device::mojom::HidReportDescription::New());

  auto device = device::mojom::HidDeviceInfo::New();
  device->guid = base::Uuid::GenerateRandomV4().AsLowercaseString();
  device->physical_device_id = "physical-" + device->guid;
  device->vendor_id = vendor_id;
  device->product_id = product_id;
  device->product_name = product_name;
  device->serial_number = serial_number;
  device->bus_type = device::mojom::HidBusType::kHIDBusTypeUSB;
  device->collections.push_back(std::move(collection));
  std::string guid = device->guid;
  for (auto& client : clients_)
    client->DeviceAdded(device->Clone());
  devices_[guid] = std::move(device);
  return guid;
}

void FakeHidManager::RemoveDevice(const std::string& guid) {
  auto it = devices_.find(guid);
  if (it == devices_.end())
    return;
  auto device = std::move(it->second);
  devices_.erase(it);
  for (auto& client : clients_)
    client->DeviceRemoved(device->Clone());
}

size_t FakeHidManager::OpenConnectionCount() {
  std::erase_if(connections_, [](const auto& c) { return c->closed(); });
  return connections_.size();
}

void FakeHidManager::GetDevicesAndSetClient(
    mojo::PendingAssociatedRemote<device::mojom::HidManagerClient> client,
    GetDevicesAndSetClientCallback callback) {
  clients_.Add(std::move(client));
  GetDevices(std::move(callback));
}

void FakeHidManager::GetDevices(GetDevicesCallback callback) {
  std::vector<device::mojom::HidDeviceInfoPtr> list;
  for (const auto& [guid, device] : devices_)
    list.push_back(device->Clone());
  std::move(callback).Run(std::move(list));
}

void FakeHidManager::Connect(
    const std::string& device_guid,
    mojo::PendingRemote<device::mojom::HidConnectionClient> connection_client,
    mojo::PendingRemote<device::mojom::HidConnectionWatcher> watcher,
    bool allow_protected_reports,
    bool allow_fido_reports,
    ConnectCallback callback) {
  if (!devices_.contains(device_guid)) {
    std::move(callback).Run(mojo::NullRemote());
    return;
  }
  std::erase_if(connections_, [](const auto& c) { return c->closed(); });
  mojo::PendingRemote<device::mojom::HidConnection> connection;
  connections_.push_back(std::make_unique<Connection>(
      connection.InitWithNewPipeAndPassReceiver(), std::move(connection_client),
      std::move(watcher)));
  std::move(callback).Run(std::move(connection));
}

void FakeHidManager::AddReceiver(
    mojo::PendingReceiver<device::mojom::HidManager> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// ---------------------------------------------------------------- USB -----

class FakeUsbDeviceManager::Device : public device::mojom::UsbDevice {
 public:
  Device(mojo::PendingReceiver<device::mojom::UsbDevice> receiver,
         mojo::PendingRemote<device::mojom::UsbDeviceClient> client)
      : receiver_(this, std::move(receiver)) {
    // Like the device service: the browser drops the client pipe to force the
    // device closed when permission is revoked.
    if (client) {
      client_.Bind(std::move(client));
      client_.set_disconnect_handler(
          base::BindOnce(&Device::OnDisconnect, base::Unretained(this)));
    }
    receiver_.set_disconnect_handler(
        base::BindOnce(&Device::OnDisconnect, base::Unretained(this)));
  }
  ~Device() override = default;

  bool closed() const { return !receiver_.is_bound(); }
  bool opened() const { return opened_; }

  // device::mojom::UsbDevice:
  void Open(OpenCallback callback) override {
    opened_ = true;
    if (client_)
      client_->OnDeviceOpened();
    std::move(callback).Run(device::mojom::UsbOpenDeviceResult::NewSuccess(
        device::mojom::UsbOpenDeviceSuccess::OK));
  }
  void Close(CloseCallback callback) override {
    if (opened_ && client_)
      client_->OnDeviceClosed();
    opened_ = false;
    std::move(callback).Run();
  }
  void SetConfiguration(uint8_t value,
                        SetConfigurationCallback callback) override {
    std::move(callback).Run(true);
  }
  void ClaimInterface(uint8_t interface_number,
                      ClaimInterfaceCallback callback) override {
    std::move(callback).Run(device::mojom::UsbClaimInterfaceResult::kSuccess);
  }
  void ReleaseInterface(uint8_t interface_number,
                        ReleaseInterfaceCallback callback) override {
    std::move(callback).Run(true);
  }
  void SetInterfaceAlternateSetting(
      uint8_t interface_number,
      uint8_t alternate_setting,
      SetInterfaceAlternateSettingCallback callback) override {
    std::move(callback).Run(true);
  }
  void Reset(ResetCallback callback) override { std::move(callback).Run(true); }
  void ClearHalt(device::mojom::UsbTransferDirection direction,
                 uint8_t endpoint_number,
                 ClearHaltCallback callback) override {
    std::move(callback).Run(true);
  }
  void ControlTransferIn(device::mojom::UsbControlTransferParamsPtr params,
                         uint32_t length,
                         uint32_t timeout,
                         ControlTransferInCallback callback) override {
    std::move(callback).Run(device::mojom::UsbTransferStatus::TRANSFER_ERROR,
                            {});
  }
  void ControlTransferOut(device::mojom::UsbControlTransferParamsPtr params,
                          base::span<const uint8_t> data,
                          uint32_t timeout,
                          ControlTransferOutCallback callback) override {
    std::move(callback).Run(device::mojom::UsbTransferStatus::TRANSFER_ERROR);
  }
  void GenericTransferIn(uint8_t endpoint_number,
                         uint32_t length,
                         uint32_t timeout,
                         GenericTransferInCallback callback) override {
    std::move(callback).Run(device::mojom::UsbTransferStatus::TRANSFER_ERROR,
                            {});
  }
  void GenericTransferOut(uint8_t endpoint_number,
                          base::span<const uint8_t> data,
                          uint32_t timeout,
                          GenericTransferOutCallback callback) override {
    std::move(callback).Run(device::mojom::UsbTransferStatus::TRANSFER_ERROR);
  }
  void IsochronousTransferIn(uint8_t endpoint_number,
                             const std::vector<uint32_t>& packet_lengths,
                             uint32_t timeout,
                             IsochronousTransferInCallback callback) override {
    std::move(callback).Run({}, {});
  }
  void IsochronousTransferOut(
      uint8_t endpoint_number,
      base::span<const uint8_t> data,
      const std::vector<uint32_t>& packet_lengths,
      uint32_t timeout,
      IsochronousTransferOutCallback callback) override {
    std::move(callback).Run({});
  }

 private:
  void OnDisconnect() {
    opened_ = false;
    receiver_.reset();
    client_.reset();
  }

  bool opened_ = false;
  mojo::Receiver<device::mojom::UsbDevice> receiver_;
  mojo::Remote<device::mojom::UsbDeviceClient> client_;
};

FakeUsbDeviceManager::FakeUsbDeviceManager() = default;
FakeUsbDeviceManager::~FakeUsbDeviceManager() = default;

mojo::PendingRemote<device::mojom::UsbDeviceManager>
FakeUsbDeviceManager::Bind() {
  mojo::PendingRemote<device::mojom::UsbDeviceManager> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

std::string FakeUsbDeviceManager::AddDevice(uint16_t vendor_id,
                                            uint16_t product_id,
                                            const std::string& product_name,
                                            const std::string& serial_number) {
  auto alternate = device::mojom::UsbAlternateInterfaceInfo::New();
  alternate->alternate_setting = 0;
  alternate->class_code = 0xff;  // vendor specific
  alternate->subclass_code = 0;
  alternate->protocol_code = 0;
  auto interface = device::mojom::UsbInterfaceInfo::New();
  interface->interface_number = 0;
  interface->alternates.push_back(std::move(alternate));
  auto config = device::mojom::UsbConfigurationInfo::New();
  config->configuration_value = 1;
  config->interfaces.push_back(std::move(interface));

  auto device = device::mojom::UsbDeviceInfo::New();
  device->guid = base::Uuid::GenerateRandomV4().AsLowercaseString();
  device->usb_version_major = 2;
  device->class_code = 0;
  device->vendor_id = vendor_id;
  device->product_id = product_id;
  device->product_name = base::UTF8ToUTF16(product_name);
  if (!serial_number.empty())
    device->serial_number = base::UTF8ToUTF16(serial_number);
  device->active_configuration = 1;
  device->configurations.push_back(std::move(config));
  std::string guid = device->guid;
  for (auto& client : clients_)
    client->OnDeviceAdded(device->Clone());
  devices_[guid] = std::move(device);
  return guid;
}

void FakeUsbDeviceManager::RemoveDevice(const std::string& guid) {
  auto it = devices_.find(guid);
  if (it == devices_.end())
    return;
  auto device = std::move(it->second);
  devices_.erase(it);
  for (auto& client : clients_)
    client->OnDeviceRemoved(device->Clone());
}

size_t FakeUsbDeviceManager::OpenConnectionCount() {
  std::erase_if(open_devices_,
                [](const auto& d) { return d->closed() || !d->opened(); });
  return open_devices_.size();
}

void FakeUsbDeviceManager::EnumerateDevicesAndSetClient(
    mojo::PendingAssociatedRemote<device::mojom::UsbDeviceManagerClient> client,
    EnumerateDevicesAndSetClientCallback callback) {
  clients_.Add(std::move(client));
  GetDevices(nullptr, std::move(callback));
}

void FakeUsbDeviceManager::GetDevices(
    device::mojom::UsbEnumerationOptionsPtr options,
    GetDevicesCallback callback) {
  std::vector<device::mojom::UsbDeviceInfoPtr> list;
  for (const auto& [guid, device] : devices_)
    list.push_back(device->Clone());
  std::move(callback).Run(std::move(list));
}

void FakeUsbDeviceManager::GetDevice(
    const std::string& guid,
    const std::vector<uint8_t>& blocked_interface_classes,
    mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
    mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client) {
  if (!devices_.contains(guid))
    return;
  std::erase_if(open_devices_, [](const auto& d) { return d->closed(); });
  open_devices_.push_back(std::make_unique<Device>(std::move(device_receiver),
                                                   std::move(device_client)));
}

void FakeUsbDeviceManager::GetUnrestrictedDevice(
    const std::string& guid,
    const std::vector<uint8_t>& blocked_interface_classes,
    mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
    mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client) {
  GetDevice(guid, blocked_interface_classes, std::move(device_receiver),
            std::move(device_client));
}

void FakeUsbDeviceManager::GetSecurityKeyDevice(
    const std::string& guid,
    mojo::PendingReceiver<device::mojom::UsbDevice> device_receiver,
    mojo::PendingRemote<device::mojom::UsbDeviceClient> device_client) {
  GetDevice(guid, {}, std::move(device_receiver), std::move(device_client));
}

void FakeUsbDeviceManager::SetClient(
    mojo::PendingAssociatedRemote<device::mojom::UsbDeviceManagerClient>
        client) {
  clients_.Add(std::move(client));
}

// ------------------------------------------------------------- Serial -----

class FakeSerialPortManager::Port : public device::mojom::SerialPort {
 public:
  Port(mojo::PendingReceiver<device::mojom::SerialPort> receiver,
       mojo::PendingRemote<device::mojom::SerialPortClient> client,
       mojo::PendingRemote<device::mojom::SerialPortConnectionWatcher> watcher)
      : receiver_(this, std::move(receiver)) {
    if (client)
      client_.Bind(std::move(client));
    if (watcher) {
      watcher_.Bind(std::move(watcher));
      watcher_.set_disconnect_handler(
          base::BindOnce(&Port::Disconnect, base::Unretained(this)));
    }
    receiver_.set_disconnect_handler(
        base::BindOnce(&Port::Disconnect, base::Unretained(this)));
  }
  ~Port() override = default;

  bool closed() const { return !receiver_.is_bound(); }

  // device::mojom::SerialPort:
  void StartWriting(mojo::ScopedDataPipeConsumerHandle consumer) override {
    consumer_ = std::move(consumer);
  }
  void StartReading(mojo::ScopedDataPipeProducerHandle producer) override {
    producer_ = std::move(producer);
  }
  void Flush(device::mojom::SerialPortFlushMode mode,
             FlushCallback callback) override {
    std::move(callback).Run();
  }
  void Drain(DrainCallback callback) override { std::move(callback).Run(); }
  void GetControlSignals(GetControlSignalsCallback callback) override {
    std::move(callback).Run(device::mojom::SerialPortControlSignals::New());
  }
  void SetControlSignals(device::mojom::SerialHostControlSignalsPtr signals,
                         SetControlSignalsCallback callback) override {
    std::move(callback).Run(true);
  }
  void ConfigurePort(device::mojom::SerialConnectionOptionsPtr options,
                     ConfigurePortCallback callback) override {
    std::move(callback).Run(true);
  }
  void GetPortInfo(GetPortInfoCallback callback) override {
    auto info = device::mojom::SerialConnectionInfo::New();
    info->bitrate = 9600;
    info->data_bits = device::mojom::SerialDataBits::EIGHT;
    info->parity_bit = device::mojom::SerialParityBit::NO_PARITY;
    info->stop_bits = device::mojom::SerialStopBits::ONE;
    std::move(callback).Run(std::move(info));
  }
  void Close(bool flush, CloseCallback callback) override {
    std::move(callback).Run();
    Disconnect();
  }

 private:
  void Disconnect() {
    consumer_.reset();
    producer_.reset();
    receiver_.reset();
    client_.reset();
    watcher_.reset();
  }

  mojo::Receiver<device::mojom::SerialPort> receiver_;
  mojo::Remote<device::mojom::SerialPortClient> client_;
  mojo::Remote<device::mojom::SerialPortConnectionWatcher> watcher_;
  mojo::ScopedDataPipeConsumerHandle consumer_;
  mojo::ScopedDataPipeProducerHandle producer_;
};

FakeSerialPortManager::FakeSerialPortManager() = default;
FakeSerialPortManager::~FakeSerialPortManager() = default;

mojo::PendingRemote<device::mojom::SerialPortManager>
FakeSerialPortManager::Bind() {
  mojo::PendingRemote<device::mojom::SerialPortManager> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

std::string FakeSerialPortManager::AddPort(const std::string& path,
                                           const std::string& display_name,
                                           uint16_t vendor_id,
                                           uint16_t product_id,
                                           const std::string& serial_number) {
  auto port = device::mojom::SerialPortInfo::New();
  port->token = base::UnguessableToken::Create();
  port->path = base::FilePath::FromUTF8Unsafe(path);
  if (!display_name.empty())
    port->display_name = display_name;
  port->has_vendor_id = true;
  port->vendor_id = vendor_id;
  port->has_product_id = true;
  port->product_id = product_id;
  if (!serial_number.empty())
    port->serial_number = serial_number;
  port->connected = true;
  base::UnguessableToken token = port->token;
  for (auto& client : clients_)
    client->OnPortAdded(port->Clone());
  ports_[token] = std::move(port);
  return token.ToString();
}

void FakeSerialPortManager::RemovePort(const std::string& token_string) {
  for (auto it = ports_.begin(); it != ports_.end(); ++it) {
    if (it->first.ToString() != token_string)
      continue;
    auto port = std::move(it->second);
    ports_.erase(it);
    for (auto& client : clients_)
      client->OnPortRemoved(port->Clone());
    return;
  }
}

void FakeSerialPortManager::SetPortConnected(const std::string& token_string,
                                             bool connected) {
  for (auto& [token, port] : ports_) {
    if (token.ToString() != token_string)
      continue;
    port->connected = connected;
    for (auto& client : clients_)
      client->OnPortConnectedStateChanged(port->Clone());
    return;
  }
}

size_t FakeSerialPortManager::OpenConnectionCount() {
  std::erase_if(open_ports_, [](const auto& p) { return p->closed(); });
  return open_ports_.size();
}

void FakeSerialPortManager::SetClient(
    mojo::PendingRemote<device::mojom::SerialPortManagerClient> client) {
  clients_.Add(std::move(client));
}

void FakeSerialPortManager::GetDevices(bool allow_bluetooth_system_prompt,
                                       GetDevicesCallback callback) {
  std::vector<device::mojom::SerialPortInfoPtr> list;
  for (const auto& [token, port] : ports_)
    list.push_back(port->Clone());
  std::move(callback).Run(std::move(list));
}

void FakeSerialPortManager::OpenPort(
    const base::UnguessableToken& token,
    bool use_alternate_path,
    device::mojom::SerialConnectionOptionsPtr options,
    mojo::PendingRemote<device::mojom::SerialPortClient> client,
    mojo::PendingRemote<device::mojom::SerialPortConnectionWatcher> watcher,
    OpenPortCallback callback) {
  if (!ports_.contains(token)) {
    std::move(callback).Run(mojo::NullRemote());
    return;
  }
  std::erase_if(open_ports_, [](const auto& p) { return p->closed(); });
  mojo::PendingRemote<device::mojom::SerialPort> port;
  open_ports_.push_back(
      std::make_unique<Port>(port.InitWithNewPipeAndPassReceiver(),
                             std::move(client), std::move(watcher)));
  std::move(callback).Run(std::move(port));
}

// ---------------------------------------------------------- Bluetooth -----

namespace {

struct FakeBluetoothState {
  std::unique_ptr<device::BluetoothAdapterFactory::GlobalOverrideValues>
      override_values;
  scoped_refptr<bluetooth::FakeCentral> central;
  mojo::Remote<bluetooth::mojom::FakeCentral> remote;
};

FakeBluetoothState& BluetoothState() {
  static base::NoDestructor<FakeBluetoothState> state;
  return *state;
}

}  // namespace

// static
void FakeBluetooth::Enable(const std::string& state_name) {
  Disable();
  auto& state = BluetoothState();
  bluetooth::mojom::CentralState central_state =
      state_name == "absent" ? bluetooth::mojom::CentralState::ABSENT
      : state_name == "powered-off"
          ? bluetooth::mojom::CentralState::POWERED_OFF
          : bluetooth::mojom::CentralState::POWERED_ON;
  state.override_values =
      device::BluetoothAdapterFactory::Get()->InitGlobalOverrideValues();
  state.override_values->SetLESupported(true);
  state.central = base::MakeRefCounted<bluetooth::FakeCentral>(
      central_state, state.remote.BindNewPipeAndPassReceiver());
  content::BluetoothAdapterFactoryWrapper::Get().SetBluetoothAdapterOverride(
      state.central);
  // Let discovery settle immediately so an unanswered chooser is observable.
  // This is process-wide and content has no way to restore the default, so
  // every later requestDevice() in this process also idles immediately; the
  // only other Bluetooth spec (chromium-spec navigator.bluetooth) accepts a
  // cancelled chooser.
  content::BluetoothDeviceChooserController::SetTestScanDurationForTesting(
      content::BluetoothDeviceChooserController::TestScanDurationSetting::
          IMMEDIATE_TIMEOUT);
}

// static
void FakeBluetooth::Disable() {
  auto& state = BluetoothState();
  if (!state.central)
    return;
  content::BluetoothAdapterFactoryWrapper::Get().SetBluetoothAdapterOverride(
      nullptr);
  state.remote.reset();
  state.central.reset();
  state.override_values.reset();
}

// static
bool FakeBluetooth::AddPeripheral(const std::string& address,
                                  const std::string& name) {
  auto& state = BluetoothState();
  if (!state.central)
    return false;
  state.central->SimulatePreconnectedPeripheral(address, name, {}, {},
                                                base::DoNothing());
  return true;
}

// -------------------------------------------------------- Registry --------

namespace {

std::map<ElectronBrowserContext*, std::unique_ptr<FakeDeviceManagers>>&
Registry() {
  static base::NoDestructor<
      std::map<ElectronBrowserContext*, std::unique_ptr<FakeDeviceManagers>>>
      registry;
  return *registry;
}

}  // namespace

// static
FakeDeviceManagers* FakeDeviceManagers::GetOrCreate(
    ElectronBrowserContext* context) {
  auto& slot = Registry()[context];
  if (!slot) {
    slot = base::WrapUnique(new FakeDeviceManagers());
    if (auto* hid = HidChooserContextFactory::GetForBrowserContext(context))
      hid->SetHidManagerForTesting(slot->hid_.Bind());
    if (auto* usb = UsbChooserContextFactory::GetForBrowserContext(context))
      usb->SetDeviceManagerForTesting(slot->usb_.Bind());
    if (auto* serial =
            SerialChooserContextFactory::GetForBrowserContext(context)) {
      serial->SetPortManagerForTesting(slot->serial_.Bind());
    }
  }
  return slot.get();
}

// static
FakeDeviceManagers* FakeDeviceManagers::Get(ElectronBrowserContext* context) {
  return base::FindPtrOrNull(Registry(), context);
}

}  // namespace electron
