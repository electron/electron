// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_SERVICES_PRINTING_PRINTER_CAPABILITIES_SERVICE_H_
#define ELECTRON_SHELL_SERVICES_PRINTING_PRINTER_CAPABILITIES_SERVICE_H_

#include <string>

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "shell/services/printing/public/mojom/printer_capabilities_service.mojom.h"

namespace electron {

class PrinterCapabilitiesService : public mojom::PrinterCapabilitiesService {
 public:
  explicit PrinterCapabilitiesService(
      mojo::PendingReceiver<mojom::PrinterCapabilitiesService> receiver);
  ~PrinterCapabilitiesService() override;

  PrinterCapabilitiesService(const PrinterCapabilitiesService&) = delete;
  PrinterCapabilitiesService& operator=(const PrinterCapabilitiesService&) =
      delete;

 private:
  void GetCapabilities(const std::string& printer_name,
                       GetCapabilitiesCallback callback) override;

  mojo::Receiver<mojom::PrinterCapabilitiesService> receiver_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_SERVICES_PRINTING_PRINTER_CAPABILITIES_SERVICE_H_
