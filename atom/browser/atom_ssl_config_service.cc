// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_ssl_config_service.h"

#include <string>

#include "base/command_line.h"
#include "atom/common/options_switches.h"
#include "content/public/browser/browser_thread.h"
#include "net/socket/ssl_client_socket.h"

namespace atom {

namespace {

uint16 GetSSLProtocolVersion(const std::string& version_string) {
  uint16 version = 0;  // Invalid
  if (version_string == "tls1")
    version = net::SSL_PROTOCOL_VERSION_TLS1;
  else if (version_string == "tls1.1")
    version = net::SSL_PROTOCOL_VERSION_TLS1_1;
  else if (version_string == "tls1.2")
    version = net::SSL_PROTOCOL_VERSION_TLS1_2;
  return version;
}

}  // namespace

AtomSSLConfigService::AtomSSLConfigService() {
  auto cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kSSLVersionFallbackMin)) {
    auto version_string =
        cmd_line->GetSwitchValueASCII(switches::kSSLVersionFallbackMin);
    config_.version_fallback_min = GetSSLProtocolVersion(version_string);
  }
}

AtomSSLConfigService::~AtomSSLConfigService() {
}

void AtomSSLConfigService::GetSSLConfig(net::SSLConfig* config) {
  *config = config_;
}

}  // namespace atom
