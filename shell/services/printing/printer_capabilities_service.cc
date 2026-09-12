// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/services/printing/printer_capabilities_service.h"

#include <utility>

#include "base/dcheck_is_on.h"
#include "shell/common/printing/printer_capabilities.h"

#if DCHECK_IS_ON()
#include "base/command_line.h"
#include "base/process/process.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#endif

namespace electron {

PrinterCapabilitiesService::PrinterCapabilitiesService(
    mojo::PendingReceiver<mojom::PrinterCapabilitiesService> receiver)
    : receiver_(this, std::move(receiver)) {}

PrinterCapabilitiesService::~PrinterCapabilitiesService() = default;

void PrinterCapabilitiesService::GetCapabilities(
    const std::string& printer_name,
    GetCapabilitiesCallback callback) {
#if DCHECK_IS_ON()
  const auto behavior =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          "electron-printer-capabilities-test-behavior");
  if (behavior == "block") {
    // Emulate a printer driver stuck in a synchronous call. The browser's
    // deadline must still fire and terminate this process.
    base::PlatformThread::Sleep(base::Minutes(5));
    return;
  }
  if (behavior == "crash") {
    base::Process::TerminateCurrentProcessImmediately(1);
  }
  if (behavior == "success") {
    std::move(callback).Run(std::string(),
                            base::DictValue()
                                .Set("inputTrays", base::ListValue())
                                .Set("mediaTypes", base::ListValue()));
    return;
  }
#endif

  auto result = GetPrinterCapabilities(printer_name);
  std::move(callback).Run(std::move(result.first), std::move(result.second));
}

}  // namespace electron
