// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_ssl_config_service.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "atom/common/options_switches.h"
#include "content/public/browser/browser_thread.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/ssl_cipher_suite_names.h"

namespace atom {

namespace {

uint16_t GetSSLProtocolVersion(const std::string& version_string) {
  uint16_t version = 0;  // Invalid
  if (version_string == "tls1")
    version = net::SSL_PROTOCOL_VERSION_TLS1;
  else if (version_string == "tls1.1")
    version = net::SSL_PROTOCOL_VERSION_TLS1_1;
  else if (version_string == "tls1.2")
    version = net::SSL_PROTOCOL_VERSION_TLS1_2;
  return version;
}

std::vector<uint16_t> ParseCipherSuites(
    const std::vector<std::string>& cipher_strings) {
  std::vector<uint16_t> cipher_suites;
  cipher_suites.reserve(cipher_strings.size());

  for (auto& cipher_string : cipher_strings) {
    uint16_t cipher_suite = 0;
    if (!net::ParseSSLCipherString(cipher_string, &cipher_suite)) {
      LOG(ERROR) << "Ignoring unrecognised cipher suite : "
                 << cipher_string;
      continue;
    }
    cipher_suites.push_back(cipher_suite);
  }
  return cipher_suites;
}

}  // namespace

AtomSSLConfigService::AtomSSLConfigService() {
  auto cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kSSLVersionFallbackMin)) {
    auto version_string =
        cmd_line->GetSwitchValueASCII(switches::kSSLVersionFallbackMin);
    config_.version_fallback_min = GetSSLProtocolVersion(version_string);
  }

  if (cmd_line->HasSwitch(switches::kCipherSuiteBlacklist)) {
    auto cipher_strings = base::SplitString(
        cmd_line->GetSwitchValueASCII(switches::kCipherSuiteBlacklist),
        ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    config_.disabled_cipher_suites = ParseCipherSuites(cipher_strings);
  }
}

AtomSSLConfigService::~AtomSSLConfigService() {
}

void AtomSSLConfigService::GetSSLConfig(net::SSLConfig* config) {
  *config = config_;
}

}  // namespace atom
