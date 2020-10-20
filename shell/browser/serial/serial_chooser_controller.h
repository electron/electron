// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_
#define SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "content/public/browser/serial_chooser.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/device/public/mojom/serial.mojom-forward.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/serial/electron_serial_delegate.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "third_party/blink/public/mojom/serial/serial.mojom.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace electron {

class ElectronSerialDelegate;

// SerialChooserController provides data for the Serial API permission prompt.
class SerialChooserController final : public SerialChooserContext::PortObserver,
                                      public content::WebContentsObserver {
 public:
  SerialChooserController(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::SerialPortFilterPtr> filters,
      content::SerialChooser::Callback callback,
      content::WebContents* web_contents,
      base::WeakPtr<ElectronSerialDelegate> serial_delegate);
  ~SerialChooserController() override;

  // SerialChooserContext::PortObserver:
  void OnPortAdded(const device::mojom::SerialPortInfo& port) override;
  void OnPortRemoved(const device::mojom::SerialPortInfo& port) override;
  void OnPortManagerConnectionError() override {}

 private:
  api::Session* GetSession();
  void OnGetDevices(std::vector<device::mojom::SerialPortInfoPtr> ports);
  bool FilterMatchesAny(const device::mojom::SerialPortInfo& port) const;
  void RunCallback(device::mojom::SerialPortInfoPtr port);
  void OnDeviceChosen(const std::string& port_id);

  std::vector<blink::mojom::SerialPortFilterPtr> filters_;
  content::SerialChooser::Callback callback_;
  url::Origin requesting_origin_;
  url::Origin embedding_origin_;

  base::WeakPtr<SerialChooserContext> chooser_context_;

  std::vector<device::mojom::SerialPortInfoPtr> ports_;

  base::WeakPtr<ElectronSerialDelegate> serial_delegate_;

  base::WeakPtrFactory<SerialChooserController> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SerialChooserController);
};

}  // namespace electron

#endif  // SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTROLLER_H_
