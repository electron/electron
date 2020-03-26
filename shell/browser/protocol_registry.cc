// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>

#include "shell/browser/electron_browser_context.h"
#include "shell/browser/protocol_registry.h"

namespace electron {

// static
ProtocolRegistry* ProtocolRegistry::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ElectronBrowserContext*>(context)->protocol_registry();
}

ProtocolRegistry::ProtocolRegistry() {}

ProtocolRegistry::~ProtocolRegistry() = default;

void ProtocolRegistry::RegisterURLLoaderFactories(
    content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories) {
  for (const auto& it : handlers_) {
    factories->emplace(it.first, std::make_unique<ElectronURLLoaderFactory>(
                                     it.second.first, it.second.second));
  }
}

bool ProtocolRegistry::RegisterProtocol(ProtocolType type,
                                        const std::string& scheme,
                                        const ProtocolHandler& handler) {
  return base::TryEmplace(handlers_, scheme, type, handler).second;
}

bool ProtocolRegistry::UnregisterProtocol(const std::string& scheme) {
  return handlers_.erase(scheme) != 0;
}

bool ProtocolRegistry::IsProtocolRegistered(const std::string& scheme) {
  return base::Contains(handlers_, scheme);
}

bool ProtocolRegistry::InterceptProtocol(ProtocolType type,
                                         const std::string& scheme,
                                         const ProtocolHandler& handler) {
  return base::TryEmplace(intercept_handlers_, scheme, type, handler).second;
}

bool ProtocolRegistry::UninterceptProtocol(const std::string& scheme) {
  return intercept_handlers_.erase(scheme) != 0;
}

bool ProtocolRegistry::IsProtocolIntercepted(const std::string& scheme) {
  return base::Contains(intercept_handlers_, scheme);
}

}  // namespace electron
