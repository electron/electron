// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/utility/electron_content_utility_client.h"

#include <utility>

#include "base/command_line.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/service_factory.h"
#include "printing/buildflags/buildflags.h"
#include "sandbox/policy/mojom/sandbox.mojom.h"
#include "sandbox/policy/sandbox_type.h"
#include "services/proxy_resolver/proxy_resolver_factory_impl.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/service.h"
#include "shell/services/node/node_service.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/services/util_win/public/mojom/util_read_icon.mojom.h"
#include "chrome/services/util_win/public/mojom/util_win.mojom.h"
#include "chrome/services/util_win/util_read_icon.h"
#include "chrome/services/util_win/util_win_impl.h"
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/services/print_compositor/print_compositor_impl.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"  // nogncheck
#endif  // BUILDFLAG(ENABLE_PRINTING)

#if BUILDFLAG(ENABLE_OOP_PRINTING)
#include "chrome/services/printing/print_backend_service_impl.h"
#include "chrome/services/printing/public/mojom/print_backend_service.mojom.h"
#endif  // BUILDFLAG(ENABLE_OOP_PRINTING)

#if BUILDFLAG(ENABLE_PRINTING) && BUILDFLAG(IS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && BUILDFLAG(IS_WIN))
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/printing_service.mojom.h"
#endif

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && BUILDFLAG(IS_WIN))
auto RunPrintingService(
    mojo::PendingReceiver<printing::mojom::PrintingService> receiver) {
  return std::make_unique<printing::PrintingService>(std::move(receiver));
}
#endif

#if BUILDFLAG(IS_WIN)
auto RunWindowsIconReader(
    mojo::PendingReceiver<chrome::mojom::UtilReadIcon> receiver) {
  return std::make_unique<UtilReadIcon>(std::move(receiver));
}
auto RunWindowsUtility(mojo::PendingReceiver<chrome::mojom::UtilWin> receiver) {
  return std::make_unique<UtilWinImpl>(std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_PRINTING)
auto RunPrintCompositor(
    mojo::PendingReceiver<printing::mojom::PrintCompositor> receiver) {
  return std::make_unique<printing::PrintCompositorImpl>(
      std::move(receiver), true /* initialize_environment */,
      content::UtilityThread::Get()->GetIOTaskRunner());
}
#endif

#if BUILDFLAG(ENABLE_OOP_PRINTING)
auto RunPrintingSandboxedPrintBackendHost(
    mojo::PendingReceiver<printing::mojom::SandboxedPrintBackendHost>
        receiver) {
  return std::make_unique<printing::SandboxedPrintBackendHostImpl>(
      std::move(receiver));
}
auto RunPrintingUnsandboxedPrintBackendHost(
    mojo::PendingReceiver<printing::mojom::UnsandboxedPrintBackendHost>
        receiver) {
  return std::make_unique<printing::UnsandboxedPrintBackendHostImpl>(
      std::move(receiver));
}
#endif  // BUILDFLAG(ENABLE_OOP_PRINTING)

auto RunProxyResolver(
    mojo::PendingReceiver<proxy_resolver::mojom::ProxyResolverFactory>
        receiver) {
  return std::make_unique<proxy_resolver::ProxyResolverFactoryImpl>(
      std::move(receiver));
}

auto RunNodeService(mojo::PendingReceiver<node::mojom::NodeService> receiver) {
  return std::make_unique<electron::NodeService>(std::move(receiver));
}

}  // namespace

ElectronContentUtilityClient::ElectronContentUtilityClient() = default;

ElectronContentUtilityClient::~ElectronContentUtilityClient() = default;

// The guts of this came from the chromium implementation
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/utility/chrome_content_utility_client.cc
void ElectronContentUtilityClient::ExposeInterfacesToBrowser(
    mojo::BinderMap* binders) {
#if BUILDFLAG(IS_WIN)
  const auto& cmd_line = *base::CommandLine::ForCurrentProcess();
  auto sandbox_type = sandbox::policy::SandboxTypeFromCommandLine(cmd_line);
  utility_process_running_elevated_ =
      sandbox_type == sandbox::mojom::Sandbox::kNoSandboxAndElevatedPrivileges;
#endif

  // If our process runs with elevated privileges, only add elevated Mojo
  // interfaces to the BinderMap.
  if (!utility_process_running_elevated_) {
#if BUILDFLAG(ENABLE_PRINTING) && BUILDFLAG(IS_WIN)
    binders->Add<printing::mojom::PdfToEmfConverterFactory>(
        base::BindRepeating(printing::PdfToEmfConverterFactory::Create),
        base::SingleThreadTaskRunner::GetCurrentDefault());
#endif
  }
}

void ElectronContentUtilityClient::RegisterMainThreadServices(
    mojo::ServiceFactory& services) {
#if BUILDFLAG(IS_WIN)
  services.Add(RunWindowsIconReader);
  services.Add(RunWindowsUtility);
#endif

#if BUILDFLAG(ENABLE_PRINTING)
  services.Add(RunPrintCompositor);
#endif

#if BUILDFLAG(ENABLE_OOP_PRINTING)
  services.Add(RunPrintingSandboxedPrintBackendHost);
  services.Add(RunPrintingUnsandboxedPrintBackendHost);
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && BUILDFLAG(IS_WIN))
  services.Add(RunPrintingService);
#endif

  services.Add(RunNodeService);
}

void ElectronContentUtilityClient::RegisterIOThreadServices(
    mojo::ServiceFactory& services) {
  services.Add(RunProxyResolver);
}

}  // namespace electron
