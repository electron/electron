// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser_context_keyed_service_factories.h"

#if defined(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "extensions/browser/api/alarms/alarm_manager.h"
#include "extensions/browser/api/api_resource_manager.h"
#include "extensions/browser/api/audio/audio_api.h"
#include "extensions/browser/api/idle/idle_manager_factory.h"
#include "extensions/browser/api/management/management_api.h"
#include "extensions/browser/api/socket/socket.h"
#include "extensions/browser/api/socket/tcp_socket.h"
#include "extensions/browser/api/socket/udp_socket.h"
#include "extensions/browser/api/sockets_tcp/tcp_socket_event_dispatcher.h"
#include "extensions/browser/api/sockets_tcp_server/tcp_server_socket_event_dispatcher.h"
#include "extensions/browser/api/sockets_udp/udp_socket_event_dispatcher.h"
#include "extensions/browser/api/storage/storage_frontend.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/declarative_user_script_manager_factory.h"
#include "extensions/browser/event_router_factory.h"
#include "extensions/browser/extension_message_filter.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/browser/renderer_startup_helper.h"
#endif

namespace atom {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
#if defined(ENABLE_EXTENSIONS)
  // keep AtomExtensionsBrowserClient::RegisterExtensionFunctions in sync
  extensions::AlarmManager::GetFactoryInstance();
  extensions::ApiResourceManager<extensions::ResumableTCPServerSocket>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::ResumableTCPSocket>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::ResumableUDPSocket>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::Socket>::GetFactoryInstance();
  extensions::AudioAPI::GetFactoryInstance();
  extensions::api::TCPServerSocketEventDispatcher::GetFactoryInstance();
  extensions::api::TCPSocketEventDispatcher::GetFactoryInstance();
  extensions::api::UDPSocketEventDispatcher::GetFactoryInstance();
  extensions::DeclarativeUserScriptManagerFactory::GetInstance();
  extensions::EventRouterFactory::GetInstance();
  extensions::ExtensionMessageFilter::EnsureShutdownNotifierFactoryBuilt();
  extensions::ExtensionPrefsFactory::GetInstance();
  extensions::IdleManagerFactory::GetInstance();
  extensions::ManagementAPI::GetFactoryInstance();
  extensions::ProcessManagerFactory::GetInstance();
  extensions::RendererStartupHelperFactory::GetInstance();
  extensions::RuntimeAPI::GetFactoryInstance();
  extensions::StorageFrontend::GetFactoryInstance();
  extensions::WebRequestAPI::GetFactoryInstance();
  extensions::AtomExtensionSystemFactory::GetInstance();
#endif
}

}  // namespace atom
