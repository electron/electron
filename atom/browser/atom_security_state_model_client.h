// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "components/security_state/security_state_model.h"
#include "components/security_state/security_state_model_client.h"
#include "content/public/browser/web_contents_user_data.h"

namespace atom {

class AtomSecurityStateModelClient
    : public security_state::SecurityStateModelClient,
      public content::WebContentsUserData<AtomSecurityStateModelClient> {
 public:
  ~AtomSecurityStateModelClient() override;

  const security_state::SecurityStateModel::SecurityInfo&
  GetSecurityInfo() const;

  // security_state::SecurityStateModelClient:
  void GetVisibleSecurityState(
      security_state::SecurityStateModel::VisibleSecurityState* state) override;
  bool RetrieveCert(scoped_refptr<net::X509Certificate>* cert) override;
  bool UsedPolicyInstalledCertificate() override;
  bool IsOriginSecure(const GURL& url) override;

 private:
  explicit AtomSecurityStateModelClient(content::WebContents* web_contents);
  friend class content::WebContentsUserData<AtomSecurityStateModelClient>;

  content::WebContents* web_contents_;
  std::unique_ptr<security_state::SecurityStateModel> security_state_model_;

  DISALLOW_COPY_AND_ASSIGN(AtomSecurityStateModelClient);
};

}  // namespace atom
