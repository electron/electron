// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_REQUIRE_CT_DELEGATE_H_
#define ATOM_BROWSER_NET_REQUIRE_CT_DELEGATE_H_

#include <set>
#include <string>

#include "net/http/transport_security_state.h"

namespace atom {

class RequireCTDelegate
    : public net::TransportSecurityState::RequireCTDelegate {
 public:
  RequireCTDelegate();
  ~RequireCTDelegate() override;

  void AddCTExcludedHost(const std::string& host);
  void ClearCTExcludedHostsList();

  // net::TransportSecurityState::RequireCTDelegate:
  CTRequirementLevel IsCTRequiredForHost(
      const std::string& host,
      const net::X509Certificate* chain,
      const net::HashValueVector& hashes) override;

 private:
  std::set<std::string> ct_excluded_hosts_;
  DISALLOW_COPY_AND_ASSIGN(RequireCTDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_REQUIRE_CT_DELEGATE_H_
