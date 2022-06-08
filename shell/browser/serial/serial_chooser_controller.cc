// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/serial/serial_chooser_controller.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/serial/serial_chooser_context.h"
#include "shell/browser/serial/serial_chooser_context_factory.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "ui/base/l10n/l10n_util.h"

namespace gin {

template <>
struct Converter<device::mojom::SerialPortInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const device::mojom::SerialPortInfoPtr& port) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("portId", port->token.ToString());
    dict.Set("portName", port->path.BaseName().LossyDisplayName());
    if (port->display_name && !port->display_name->empty()) {
      dict.Set("displayName", *port->display_name);
    }
    if (port->has_vendor_id) {
      dict.Set("vendorId", base::StringPrintf("%u", port->vendor_id));
    }
    if (port->has_product_id) {
      dict.Set("productId", base::StringPrintf("%u", port->product_id));
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

SerialChooserController::SerialChooserController(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::SerialPortFilterPtr> filters,
    content::SerialChooser::Callback callback,
    content::WebContents* web_contents,
    base::WeakPtr<ElectronSerialDelegate> serial_delegate)
    : WebContentsObserver(web_contents),
      filters_(std::move(filters)),
      callback_(std::move(callback)),
      serial_delegate_(serial_delegate),
      render_frame_host_id_(render_frame_host->GetGlobalId()) {
  origin_ = web_contents->GetMainFrame()->GetLastCommittedOrigin();

  chooser_context_ = SerialChooserContextFactory::GetForBrowserContext(
                         web_contents->GetBrowserContext())
                         ->AsWeakPtr();
  DCHECK(chooser_context_);
  chooser_context_->GetPortManager()->GetDevices(base::BindOnce(
      &SerialChooserController::OnGetDevices, weak_factory_.GetWeakPtr()));
}

SerialChooserController::~SerialChooserController() {
  RunCallback(/*port=*/nullptr);
  if (chooser_context_) {
    chooser_context_->RemovePortObserver(this);
  }
}

api::Session* SerialChooserController::GetSession() {
  if (!web_contents()) {
    return nullptr;
  }
  return api::Session::FromBrowserContext(web_contents()->GetBrowserContext());
}

void SerialChooserController::OnPortAdded(
    const device::mojom::SerialPortInfo& port) {
  ports_.push_back(port.Clone());
  api::Session* session = GetSession();
  if (session) {
    session->Emit("serial-port-added", port.Clone(), web_contents());
  }
}

void SerialChooserController::OnPortRemoved(
    const device::mojom::SerialPortInfo& port) {
  const auto it = std::find_if(
      ports_.begin(), ports_.end(),
      [&port](const auto& ptr) { return ptr->token == port.token; });
  if (it != ports_.end()) {
    api::Session* session = GetSession();
    if (session) {
      session->Emit("serial-port-removed", port.Clone(), web_contents());
    }
    ports_.erase(it);
  }
}

void SerialChooserController::OnPortManagerConnectionError() {
  // TODO(nornagon/jkleinsc): report event
}

void SerialChooserController::OnDeviceChosen(const std::string& port_id) {
  if (port_id.empty()) {
    RunCallback(/*port=*/nullptr);
  } else {
    const auto it =
        std::find_if(ports_.begin(), ports_.end(), [&port_id](const auto& ptr) {
          return ptr->token.ToString() == port_id;
        });
    if (it != ports_.end()) {
      auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
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
  std::sort(ports.begin(), ports.end(),
            [](const auto& port1, const auto& port2) {
              return port1->path.BaseName() < port2->path.BaseName();
            });

  for (auto& port : ports) {
    if (FilterMatchesAny(*port))
      ports_.push_back(std::move(port));
  }

  bool prevent_default = false;
  api::Session* session = GetSession();
  if (session) {
    prevent_default =
        session->Emit("select-serial-port", ports_, web_contents(),
                      base::AdaptCallbackForRepeating(base::BindOnce(
                          &SerialChooserController::OnDeviceChosen,
                          weak_factory_.GetWeakPtr())));
  }
  if (!prevent_default) {
    RunCallback(/*port=*/nullptr);
  }
}

bool SerialChooserController::FilterMatchesAny(
    const device::mojom::SerialPortInfo& port) const {
  if (filters_.empty())
    return true;

  for (const auto& filter : filters_) {
    if (filter->has_vendor_id &&
        (!port.has_vendor_id || filter->vendor_id != port.vendor_id)) {
      continue;
    }
    if (filter->has_product_id &&
        (!port.has_product_id || filter->product_id != port.product_id)) {
      continue;
    }
    return true;
  }

  return false;
}

void SerialChooserController::RunCallback(
    device::mojom::SerialPortInfoPtr port) {
  if (callback_) {
    std::move(callback_).Run(std::move(port));
  }
}

}  // namespace electron
