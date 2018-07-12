// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/utility/atom_content_utility_client.h"

#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/proxy/mojo_proxy_resolver_factory_impl.h"
#include "services/service_manager/public/cpp/binder_registry.h"

#if defined(OS_WIN)
#include "base/memory/ptr_util.h"
#include "chrome/utility/printing_handler_win.h"
#endif

namespace atom {

namespace {

void CreateProxyResolverFactory(
    net::interfaces::ProxyResolverFactoryRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<net::MojoProxyResolverFactoryImpl>(),
                          std::move(request));
}

}  // namespace

AtomContentUtilityClient::AtomContentUtilityClient() {
#if defined(OS_WIN)
  handlers_.push_back(base::MakeUnique<printing::PrintingHandlerWin>());
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() {
}

void AtomContentUtilityClient::UtilityThreadStarted() {
  content::ServiceManagerConnection* connection =
      content::ChildThread::Get()->GetServiceManagerConnection();

  // NOTE: Some utility process instances are not connected to the Service
  // Manager. Nothing left to do in that case.
  if (!connection)
    return;

  auto registry = base::MakeUnique<service_manager::BinderRegistry>();
  registry->AddInterface<net::interfaces::ProxyResolverFactory>(
      base::Bind(CreateProxyResolverFactory),
      base::ThreadTaskRunnerHandle::Get());
  connection->AddConnectionFilter(
      base::MakeUnique<content::SimpleConnectionFilter>(std::move(registry)));
}

bool AtomContentUtilityClient::OnMessageReceived(
    const IPC::Message& message) {
#if defined(OS_WIN)
  for (const auto& handler : handlers_) {
    if (handler->OnMessageReceived(message))
      return true;
  }
#endif

  return false;
}

}  // namespace atom
