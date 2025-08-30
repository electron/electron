// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_
#define ELECTRON_SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/serial_chooser.h"
#include "content/public/browser/weak_document_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "services/device/public/mojom/serial.mojom-forward.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/mojom/serial/serial.mojom-forward.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace gin {
template <typename T>
class WeakCell;
}  // namespace gin

namespace electron {

namespace api {
class Session;
}

class ElectronSerialDelegate;

// SerialChooserController provides data for the Serial API permission prompt.
class SerialChooserController final
    : private SerialChooserContext::PortObserver,
      private device::BluetoothAdapter::Observer {
 public:
  SerialChooserController(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::SerialPortFilterPtr> filters,
      std::vector<::device::BluetoothUUID> allowed_bluetooth_service_class_ids,
      content::SerialChooser::Callback callback,
      content::WebContents* web_contents,
      base::WeakPtr<ElectronSerialDelegate> serial_delegate);
  ~SerialChooserController() override;

  // disable copy
  SerialChooserController(const SerialChooserController&) = delete;
  SerialChooserController& operator=(const SerialChooserController&) = delete;

  // SerialChooserContext::PortObserver:
  void OnPortAdded(const device::mojom::SerialPortInfo& port) override;
  void OnPortRemoved(const device::mojom::SerialPortInfo& port) override;
  void OnPortConnectedStateChanged(
      const device::mojom::SerialPortInfo& port) override {}
  void OnPortManagerConnectionError() override;
  void OnPermissionRevoked(const url::Origin& origin) override {}
  void OnSerialChooserContextShutdown() override;

  // BluetoothAdapter::Observer
  void AdapterPoweredChanged(device::BluetoothAdapter* adapter,
                             bool powered) override;

 private:
  gin::WeakCell<api::Session>* GetSession();
  void GetDevices();
  void OnGetDevices(std::vector<device::mojom::SerialPortInfoPtr> ports);
  bool DisplayDevice(const device::mojom::SerialPortInfo& port) const;
  void AddMessageToConsole(blink::mojom::ConsoleMessageLevel level,
                           const std::string& message) const;
  void RunCallback(device::mojom::SerialPortInfoPtr port);
  void OnDeviceChosen(const std::string& port_id);
  void OnGetAdapter(base::OnceClosure callback,
                    scoped_refptr<device::BluetoothAdapter> adapter);
  // Whether it will only show ports from bluetooth devices.
  [[nodiscard]] bool IsWirelessSerialPortOnly() const;

  base::WeakPtr<content::WebContents> web_contents_;

  std::vector<blink::mojom::SerialPortFilterPtr> filters_;
  std::vector<::device::BluetoothUUID> allowed_bluetooth_service_class_ids_;
  content::SerialChooser::Callback callback_;
  content::WeakDocumentPtr initiator_document_;
  url::Origin origin_;

  base::WeakPtr<SerialChooserContext> chooser_context_;

  base::ScopedObservation<SerialChooserContext,
                          SerialChooserContext::PortObserver>
      observation_{this};

  std::vector<device::mojom::SerialPortInfoPtr> ports_;

  scoped_refptr<device::BluetoothAdapter> adapter_;
  base::ScopedObservation<device::BluetoothAdapter,
                          device::BluetoothAdapter::Observer>
      adapter_observation_{this};

  base::WeakPtr<ElectronSerialDelegate> serial_delegate_;
  content::GlobalRenderFrameHostId render_frame_host_id_;

  base::WeakPtrFactory<SerialChooserController> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_
