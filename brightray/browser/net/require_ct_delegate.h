// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NET_REQUIRE_CT_DELEGATE_H_
#define BRIGHTRAY_BROWSER_NET_REQUIRE_CT_DELEGATE_H_

#include <set>
#include <string>

#include "net/http/transport_security_state.h"

namespace brightray {

class RequireCTDelegate
    : public net::TransportSecurityState::RequireCTDelegate {
 public:
  RequireCTDelegate();
  ~RequireCTDelegate() override;

  void AddCTExcludedHost(const std::string& host);
  void ClearCTExcludedHostsList();

  // net::TransportSecurityState::RequireCTDelegate:
  CTRequirementLevel IsCTRequiredForHost(const std::string& host) override;

 private:
  std::set<std::string> ct_excluded_hosts_;
  DISALLOW_COPY_AND_ASSIGN(RequireCTDelegate);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NET_REQUIRE_CT_DELEGATE_H_
