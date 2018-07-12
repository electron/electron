// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
#define BRIGHTRAY_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_

#include <stddef.h>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"
#include "net/proxy/mojo_proxy_resolver_factory.h"

namespace content {
class UtilityProcessHost;
}

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}  // namespace base

// A factory used to create connections to Mojo proxy resolver services.  On
// Android, the proxy resolvers will run in the browser process, and on other
// platforms, they'll all be run in the same utility process. Utility process
// crashes are detected and the utility process is automatically restarted.
class ChromeMojoProxyResolverFactory : public net::MojoProxyResolverFactory {
 public:
  static ChromeMojoProxyResolverFactory* GetInstance();

  // Overridden from net::MojoProxyResolverFactory:
  std::unique_ptr<base::ScopedClosureRunner> CreateResolver(
      const std::string& pac_script,
      mojo::InterfaceRequest<net::interfaces::ProxyResolver> req,
      net::interfaces::ProxyResolverFactoryRequestClientPtr client) override;

 private:
  friend struct base::DefaultSingletonTraits<ChromeMojoProxyResolverFactory>;
  ChromeMojoProxyResolverFactory();
  ~ChromeMojoProxyResolverFactory() override;

  // Creates the proxy resolver factory. On desktop, creates a new utility
  // process before creating it out of process. On Android, creates it on the
  // current thread.
  void CreateFactory();

  // Destroys |resolver_factory_|.
  void DestroyFactory();

  // Invoked each time a proxy resolver is destroyed.
  void OnResolverDestroyed();

  // Invoked once an idle timeout has elapsed after all proxy resolvers are
  // destroyed.
  void OnIdleTimeout();

  net::interfaces::ProxyResolverFactoryPtr resolver_factory_;

  base::WeakPtr<content::UtilityProcessHost> weak_utility_process_host_;

  size_t num_proxy_resolvers_ = 0;

  base::OneShotTimer idle_timer_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMojoProxyResolverFactory);
};

#endif  // BRIGHTRAY_BROWSER_NET_CHROME_MOJO_PROXY_RESOLVER_FACTORY_H_
