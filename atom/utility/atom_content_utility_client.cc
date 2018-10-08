// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/utility/atom_content_utility_client.h"

#include "base/command_line.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "services/proxy_resolver/proxy_resolver_service.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/sandbox/switches.h"

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/constants.mojom.h"
#include "chrome/utility/printing_handler.h"
#endif

namespace atom {

AtomContentUtilityClient::AtomContentUtilityClient() : elevated_(false) {
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
  printing_handler_ = std::make_unique<printing::PrintingHandler>();
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() {}

void AtomContentUtilityClient::UtilityThreadStarted() {
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
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

void AtomContentUtilityClient::RegisterServices(StaticServiceMap* services) {
  service_manager::EmbeddedServiceInfo proxy_resolver_info;
  proxy_resolver_info.task_runner =
      content::ChildThread::Get()->GetIOTaskRunner();
  proxy_resolver_info.factory =
      base::BindRepeating(&proxy_resolver::ProxyResolverService::CreateService);
  services->emplace(proxy_resolver::mojom::kProxyResolverServiceName,
                    proxy_resolver_info);

#if BUILDFLAG(ENABLE_PRINTING)
  service_manager::EmbeddedServiceInfo printing_info;
  printing_info.factory =
      base::BindRepeating(&printing::PrintingService::CreateService);
  services->emplace(printing::mojom::kChromePrintingServiceName, printing_info);
#endif
}

}  // namespace atom
