// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/utility/atom_content_utility_client.h"

#include <utility>

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/service_factory.h"
#include "services/proxy_resolver/proxy_resolver_factory_impl.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/sandbox/switches.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/services/pdf_compositor/pdf_compositor_impl.h"
#include "components/services/pdf_compositor/public/mojom/pdf_compositor.mojom.h"

#if defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/printing_service.mojom.h"
#include "chrome/utility/printing_handler.h"
#endif  // defined(OS_WIN)

#endif  // BUILDFLAG(ENABLE_PRINTING)

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
auto RunPrintingService(
    mojo::PendingReceiver<printing::mojom::PrintingService> receiver) {
  return std::make_unique<printing::PrintingService>(std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_PRINTING)
auto RunPdfCompositor(
    mojo::PendingReceiver<printing::mojom::PdfCompositor> receiver) {
  return std::make_unique<printing::PdfCompositorImpl>(
      std::move(receiver), true /* initialize_environment */,
      content::UtilityThread::Get()->GetIOTaskRunner());
}
#endif

auto RunProxyResolver(
    mojo::PendingReceiver<proxy_resolver::mojom::ProxyResolverFactory>
        receiver) {
  return std::make_unique<proxy_resolver::ProxyResolverFactoryImpl>(
      std::move(receiver));
}

}  // namespace

AtomContentUtilityClient::AtomContentUtilityClient() : elevated_(false) {
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
  printing_handler_ = std::make_unique<printing::PrintingHandler>();
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() {}

// The guts of this came from the chromium implementation
// https://cs.chromium.org/chromium/src/chrome/utility/
// chrome_content_utility_client.cc?sq=package:chromium&dr=CSs&g=0&l=142
void AtomContentUtilityClient::UtilityThreadStarted() {
#if defined(OS_WIN)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  elevated_ = command_line->HasSwitch(
      service_manager::switches::kNoSandboxAndElevatedPrivileges);
#endif

  content::ServiceManagerConnection* connection =
      content::ChildThread::Get()->GetServiceManagerConnection();

  // NOTE: Some utility process instances are not connected to the Service
  // Manager. Nothing left to do in that case.
  if (!connection)
    return;

  auto registry = std::make_unique<service_manager::BinderRegistry>();
  // If our process runs with elevated privileges, only add elevated Mojo
  // interfaces to the interface registry.
  if (!elevated_) {
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
    // TODO(crbug.com/798782): remove when the Cloud print chrome/service is
    // removed.
    registry->AddInterface(
        base::BindRepeating(printing::PdfToEmfConverterFactory::Create),
        base::ThreadTaskRunnerHandle::Get());
#endif
  }

  connection->AddConnectionFilter(
      std::make_unique<content::SimpleConnectionFilter>(std::move(registry)));
}

bool AtomContentUtilityClient::OnMessageReceived(const IPC::Message& message) {
  if (elevated_)
    return false;

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
  if (printing_handler_->OnMessageReceived(message))
    return true;
#endif

  return false;
}

mojo::ServiceFactory* AtomContentUtilityClient::GetMainThreadServiceFactory() {
  static base::NoDestructor<mojo::ServiceFactory> factory {
#if BUILDFLAG(ENABLE_PRINTING)
    RunPdfCompositor,
#if defined(OS_WIN)
        RunPrintingService
#endif
#endif
  };
  return factory.get();
}

mojo::ServiceFactory* AtomContentUtilityClient::GetIOThreadServiceFactory() {
  static base::NoDestructor<mojo::ServiceFactory> factory{RunProxyResolver};
  return factory.get();
}

}  // namespace electron
