// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_NET_ELECTRON_SSL_CONFIG_SERVICE_H_
#define ELECTRON_BROWSER_NET_ELECTRON_SSL_CONFIG_SERVICE_H_

#include "net/ssl/ssl_config_service.h"

namespace electron {

class AtomSSLConfigService : public net::SSLConfigService {
 public:
  AtomSSLConfigService();
  ~AtomSSLConfigService() override;

  // net::SSLConfigService:
  void GetSSLConfig(net::SSLConfig* config) override;

 private:
  net::SSLConfig config_;

  DISALLOW_COPY_AND_ASSIGN(AtomSSLConfigService);
};

}   // namespace electron

#endif  // ELECTRON_BROWSER_NET_ELECTRON_SSL_CONFIG_SERVICE_H_
