// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_CT_DELEGATE_H_
#define ATOM_BROWSER_NET_ATOM_CT_DELEGATE_H_

#include <set>
#include <string>

#include "net/http/transport_security_state.h"

namespace atom {

class AtomCTDelegate : public net::TransportSecurityState::RequireCTDelegate {
 public:
  AtomCTDelegate();
  ~AtomCTDelegate() override;

  void AddCTExcludedHost(const std::string& host);
  void ClearCTExcludedHostsList();

  // net::TransportSecurityState::RequireCTDelegate:
  CTRequirementLevel IsCTRequiredForHost(const std::string& host) override;

 private:
  std::set<std::string> ct_excluded_hosts_;
  DISALLOW_COPY_AND_ASSIGN(AtomCTDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_CT_DELEGATE_H_
