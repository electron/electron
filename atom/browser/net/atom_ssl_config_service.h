// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "net/ssl/ssl_config_service.h"

namespace atom {

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

}   // namespace atom
