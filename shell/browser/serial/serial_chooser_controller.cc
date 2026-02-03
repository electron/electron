// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_controller.h"

#include <algorithm>
#include <utility>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/serial/serial_blocklist.h"
#include "content/public/browser/console_message.h"
#include "content/public/browser/web_contents.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/public/cpp/bluetooth_uuid.h"
#include "services/device/public/cpp/bluetooth/bluetooth_utils.h"
#include "services/device/public/mojom/serial.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/serial/electron_serial_delegate.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "ui/base/l10n/l10n_util.h"

namespace gin {

template <>
struct Converter<device::mojom::SerialPortInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const device::mojom::SerialPortInfoPtr& port) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("portId", port->token.ToString());
    dict.Set("portName", port->path.BaseName().LossyDisplayName());
    if (port->display_name && !port->display_name->empty()) {
      dict.Set("displayName", *port->display_name);
    }
    if (port->has_vendor_id) {
      dict.Set("vendorId", base::NumberToString(port->vendor_id));
    }
    if (port->has_product_id) {
      dict.Set("productId", base::NumberToString(port->product_id));
    }
    if (port->serial_number && !port->serial_number->empty()) {
      dict.Set("serialNumber", *port->serial_number);
    }
#if BUILDFLAG(IS_MAC)
    if (port->usb_driver_name && !port->usb_driver_name->empty()) {
      dict.Set("usbDriverName", *port->usb_driver_name);
    }
#elif BUILDFLAG(IS_WIN)
    if (!port->device_instance_id.empty()) {
      dict.Set("deviceInstanceId", port->device_instance_id);
    }
#endif
    return gin::ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron {

namespace {

using ::device::BluetoothAdapter;
using ::device::BluetoothAdapterFactory;
using ::device::mojom::SerialPortType;

bool FilterMatchesPort(const blink::mojom::SerialPortFilter& filter,
                       const device::mojom::SerialPortInfo& port) {
  if (filter.bluetooth_service_class_id) {
    if (!port.bluetooth_service_class_id) {
      return false;
    }
    return device::BluetoothUUID(*port.bluetooth_service_class_id) ==
           device::BluetoothUUID(*filter.bluetooth_service_class_id);
  }
  if (!filter.has_vendor_id) {
    return true;
  }
  if (!port.has_vendor_id || port.vendor_id != filter.vendor_id) {
    return false;
  }
  if (!filter.has_product_id) {
    return true;
  }
  return port.has_product_id && port.product_id == filter.product_id;
}

bool BluetoothPortIsAllowed(
    const std::vector<::device::BluetoothUUID>& allowed_ids,
    const device::mojom::SerialPortInfo& port) {
  if (!port.bluetooth_service_class_id) {
    return true;
  }
  // Serial Port Profile is allowed by default.
  if (*port.bluetooth_service_class_id == device::GetSerialPortProfileUUID()) {
    return true;
  }
  return std::ranges::contains(allowed_ids,
                               port.bluetooth_service_class_id.value());
}

}  // namespace

SerialChooserController::SerialChooserController(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    std::vector<::device::BluetoothUUID> allowed_bluetooth_service_class_ids,
    content::SerialChooser::Callback callback,
    content::WebContents* web_contents,
    base::WeakPtr<ElectronSerialDelegate> serial_delegate)
    : web_contents_{web_contents ? web_contents->GetWeakPtr()
                                 : base::WeakPtr<content::WebContents>()},
      filters_(std::move(filters)),
      allowed_bluetooth_service_class_ids_(
          std::move(allowed_bluetooth_service_class_ids)),
      callback_(std::move(callback)),
      initiator_document_(render_frame_host->GetWeakDocumentPtr()) {
  origin_ = web_contents_->GetPrimaryMainFrame()->GetLastCommittedOrigin();

  chooser_context_ = SerialChooserContextFactory::GetForBrowserContext(
                         web_contents_->GetBrowserContext())
                         ->AsWeakPtr();
  DCHECK(chooser_context_);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&SerialChooserController::GetDevices,
                                weak_factory_.GetWeakPtr()));

  observation_.Observe(chooser_context_.get());
}

SerialChooserController::~SerialChooserController() {
  RunCallback(/*port=*/nullptr);
}

gin::WeakCell<api::Session>* SerialChooserController::GetSession() {
  if (!web_contents_) {
    return nullptr;
  }
  return api::Session::FromBrowserContext(web_contents_->GetBrowserContext());
}

void SerialChooserController::GetDevices() {
  if (IsWirelessSerialPortOnly()) {
    if (!adapter_) {
      BluetoothAdapterFactory::Get()->GetAdapter(base::BindOnce(
          &SerialChooserController::OnGetAdapter, weak_factory_.GetWeakPtr(),
          base::BindOnce(&SerialChooserController::GetDevices,
                         weak_factory_.GetWeakPtr())));
      return;
    }
  }

  chooser_context_->GetPortManager()->GetDevices(base::BindOnce(
      &SerialChooserController::OnGetDevices, weak_factory_.GetWeakPtr()));
}

void SerialChooserController::AdapterPoweredChanged(BluetoothAdapter* adapter,
                                                    bool powered) {
  // TODO(codebytere): maybe emit an event here?
  if (powered) {
    GetDevices();
  }
}

void SerialChooserController::OnPortAdded(
    const device::mojom::SerialPortInfo& port) {
  if (!DisplayDevice(port))
    return;

  ports_.push_back(port.Clone());

  gin::WeakCell<api::Session>* session = GetSession();
  if (session && session->Get()) {
    session->Get()->Emit("serial-port-added", port.Clone(),
                         web_contents_.get());
  }
}

void SerialChooserController::OnPortRemoved(
    const device::mojom::SerialPortInfo& port) {
  const auto it = std::ranges::find(ports_, port.token,
                                    &device::mojom::SerialPortInfo::token);
  if (it != ports_.end()) {
    gin::WeakCell<api::Session>* session = GetSession();
    if (session && session->Get()) {
      session->Get()->Emit("serial-port-removed", port.Clone(),
                           web_contents_.get());
    }
    ports_.erase(it);
  }
}

void SerialChooserController::OnPortManagerConnectionError() {
  observation_.Reset();
}

void SerialChooserController::OnSerialChooserContextShutdown() {
  observation_.Reset();
}

void SerialChooserController::OnDeviceChosen(const std::string& port_id) {
  if (port_id.empty()) {
    RunCallback(/*port=*/nullptr);
  } else {
    const auto it = std::ranges::find_if(ports_, [&port_id](const auto& ptr) {
      return ptr->token.ToString() == port_id;
    });
    if (it != ports_.end()) {
      auto* rfh = initiator_document_.AsRenderFrameHostIfValid();
      chooser_context_->GrantPortPermission(origin_, *it->get(), rfh);
      RunCallback(it->Clone());
    } else {
      RunCallback(/*port=*/nullptr);
    }
  }
}

void SerialChooserController::OnGetDevices(
    std::vector<device::mojom::SerialPortInfoPtr> ports) {
  // Sort ports by file paths.
  std::ranges::sort(ports, [](const auto& port1, const auto& port2) {
    return port1->path.BaseName() < port2->path.BaseName();
  });

  ports_.clear();
  for (auto& port : ports) {
    if (DisplayDevice(*port))
      ports_.push_back(std::move(port));
  }

  bool prevent_default = false;
  gin::WeakCell<api::Session>* session = GetSession();
  if (session && session->Get()) {
    prevent_default = session->Get()->Emit(
        "select-serial-port", ports_, web_contents_.get(),
        base::BindRepeating(&SerialChooserController::OnDeviceChosen,
                            weak_factory_.GetWeakPtr()));
  }
  if (!prevent_default) {
    RunCallback(/*port=*/nullptr);
  }
}

bool SerialChooserController::DisplayDevice(
    const device::mojom::SerialPortInfo& port) const {
  bool blocklist_disabled = base::CommandLine::ForCurrentProcess()->HasSwitch(
      kDisableSerialBlocklist);
  if (!blocklist_disabled && SerialBlocklist::Get().IsExcluded(port)) {
    if (port.has_vendor_id && port.has_product_id) {
      AddMessageToConsole(
          blink::mojom::ConsoleMessageLevel::kInfo,
          base::StringPrintf(
              "Skipping a port blocked by "
              "the Serial blocklist: vendorId=%d, "
              "productId=%d, name='%s', serial='%s'",
              port.vendor_id, port.product_id,
              port.display_name ? port.display_name.value().c_str() : "",
              port.serial_number ? port.serial_number.value().c_str() : ""));
    } else if (port.bluetooth_service_class_id) {
      AddMessageToConsole(
          blink::mojom::ConsoleMessageLevel::kInfo,
          base::StringPrintf(
              "Skipping a port blocked by "
              "the Serial blocklist: bluetoothServiceClassId=%s, "
              "name='%s'",
              port.bluetooth_service_class_id->value().c_str(),
              port.display_name ? port.display_name.value().c_str() : ""));
    } else {
      NOTREACHED();
    }
    return false;
  }

  if (filters_.empty()) {
    return BluetoothPortIsAllowed(allowed_bluetooth_service_class_ids_, port);
  }

  for (const auto& filter : filters_) {
    if (FilterMatchesPort(*filter, port) &&
        BluetoothPortIsAllowed(allowed_bluetooth_service_class_ids_, port)) {
      return true;
    }
  }

  return false;
}

void SerialChooserController::AddMessageToConsole(
    blink::mojom::ConsoleMessageLevel level,
    const std::string& message) const {
  if (content::RenderFrameHost* rfh =
          initiator_document_.AsRenderFrameHostIfValid()) {
    rfh->AddMessageToConsole(level, message);
  }
}

void SerialChooserController::RunCallback(
    device::mojom::SerialPortInfoPtr port) {
  if (callback_) {
    std::move(callback_).Run(std::move(port));
  }
}
void SerialChooserController::OnGetAdapter(
    base::OnceClosure callback,
    scoped_refptr<BluetoothAdapter> adapter) {
  CHECK(adapter);
  adapter_ = std::move(adapter);
  adapter_observation_.Observe(adapter_.get());
  std::move(callback).Run();
}

bool SerialChooserController::IsWirelessSerialPortOnly() const {
  if (allowed_bluetooth_service_class_ids_.empty()) {
    return false;
  }

  // The system's wired and wireless serial ports can be shown if there is no
  // filter.
  if (filters_.empty()) {
    return false;
  }

  // Check if all the filters are meant for serial port from Bluetooth device.
  for (const auto& filter : filters_) {
    if (!filter->bluetooth_service_class_id) {
      return false;
    }
  }
  return true;
}

}  // namespace electron
