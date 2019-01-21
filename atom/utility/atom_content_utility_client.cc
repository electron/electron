// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/utility/atom_content_utility_client.h"

#include <utility>

#include "base/command_line.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/utility/utility_thread.h"
#include "services/proxy_resolver/proxy_resolver_service.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/sandbox/switches.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/constants.mojom.h"
#include "components/services/pdf_compositor/public/cpp/pdf_compositor_service_factory.h"
#include "components/services/pdf_compositor/public/interfaces/pdf_compositor.mojom.h"

#if defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#include "chrome/utility/printing_handler.h"
#endif  // defined(OS_WIN)

#endif  // BUILDFLAG(ENABLE_PRINTING)

namespace atom {

namespace {

void RunServiceAsyncThenTerminateProcess(
    std::unique_ptr<service_manager::Service> service) {
  service_manager::Service::RunAsyncUntilTermination(
      std::move(service),
      base::BindOnce([] { content::UtilityThread::Get()->ReleaseProcess(); }));
}

std::unique_ptr<service_manager::Service> CreateProxyResolverService(
    service_manager::mojom::ServiceRequest request) {
  return std::make_unique<proxy_resolver::ProxyResolverService>(
      std::move(request));
}

using ServiceFactory =
    base::OnceCallback<std::unique_ptr<service_manager::Service>()>;
void RunServiceOnIOThread(ServiceFactory factory) {
  base::OnceClosure terminate_process = base::BindOnce(
      base::IgnoreResult(&base::SequencedTaskRunner::PostTask),
      base::SequencedTaskRunnerHandle::Get(), FROM_HERE,
      base::BindOnce([] { content::UtilityThread::Get()->ReleaseProcess(); }));
  content::ChildThread::Get()->GetIOTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](ServiceFactory factory, base::OnceClosure terminate_process) {
            service_manager::Service::RunAsyncUntilTermination(
                std::move(factory).Run(), std::move(terminate_process));
          },
          std::move(factory), std::move(terminate_process)));
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

bool AtomContentUtilityClient::HandleServiceRequest(
    const std::string& service_name,
    service_manager::mojom::ServiceRequest request) {
  if (service_name == proxy_resolver::mojom::kProxyResolverServiceName) {
    RunServiceOnIOThread(
        base::BindOnce(&CreateProxyResolverService, std::move(request)));
    return true;
  }

  auto service = MaybeCreateMainThreadService(service_name, std::move(request));
  if (service) {
    RunServiceAsyncThenTerminateProcess(std::move(service));
    return true;
  }

  return false;
}

std::unique_ptr<service_manager::Service>
AtomContentUtilityClient::MaybeCreateMainThreadService(
    const std::string& service_name,
    service_manager::mojom::ServiceRequest request) {
#if BUILDFLAG(ENABLE_PRINTING)
  if (service_name == printing::mojom::kServiceName) {
    return printing::CreatePdfCompositorService(std::move(request));
  }

  if (service_name == printing::mojom::kChromePrintingServiceName) {
    return std::make_unique<printing::PrintingService>(std::move(request));
  }
#endif

  return nullptr;
}

}  // namespace atom
