// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_security_state_model_client.h"

#include "content/public/browser/cert_store.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/ssl_status.h"
#include "net/cert/x509_certificate.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::AtomSecurityStateModelClient);

using security_state::SecurityStateModel;

namespace atom {

namespace {

SecurityStateModel::SecurityLevel GetSecurityLevelForSecurityStyle(
    content::SecurityStyle style) {
  switch (style) {
    case content::SECURITY_STYLE_UNKNOWN:
      return SecurityStateModel::NONE;
    case content::SECURITY_STYLE_UNAUTHENTICATED:
      return SecurityStateModel::NONE;
    case content::SECURITY_STYLE_AUTHENTICATION_BROKEN:
      return SecurityStateModel::SECURITY_ERROR;
    case content::SECURITY_STYLE_WARNING:
      return SecurityStateModel::SECURITY_WARNING;
    case content::SECURITY_STYLE_AUTHENTICATED:
      return SecurityStateModel::SECURE;
  }
  return SecurityStateModel::NONE;
}

}  // namespace

AtomSecurityStateModelClient::AtomSecurityStateModelClient(
    content::WebContents* web_contents)
    : web_contents_(web_contents),
      security_state_model_(new SecurityStateModel()) {
  security_state_model_->SetClient(this);
}

AtomSecurityStateModelClient::~AtomSecurityStateModelClient() {
}

const SecurityStateModel::SecurityInfo&
AtomSecurityStateModelClient::GetSecurityInfo() const {
  return security_state_model_->GetSecurityInfo();
}

bool AtomSecurityStateModelClient::RetrieveCert(
    scoped_refptr<net::X509Certificate>* cert) {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  if (!entry)
    return false;
  return content::CertStore::GetInstance()->RetrieveCert(
      entry->GetSSL().cert_id, cert);
}

bool AtomSecurityStateModelClient::UsedPolicyInstalledCertificate() {
  return false;
}

bool AtomSecurityStateModelClient::IsOriginSecure(const GURL& url) {
  return content::IsOriginSecure(url);
}

void AtomSecurityStateModelClient::GetVisibleSecurityState(
    SecurityStateModel::VisibleSecurityState* state) {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  if (!entry ||
      entry->GetSSL().security_style == content::SECURITY_STYLE_UNKNOWN) {
    *state = SecurityStateModel::VisibleSecurityState();
    return;
  }

  state->initialized = true;
  state->url = entry->GetURL();
  const content::SSLStatus& ssl = entry->GetSSL();
  state->initial_security_level =
      GetSecurityLevelForSecurityStyle(ssl.security_style);
  state->cert_id = ssl.cert_id;
  state->cert_status = ssl.cert_status;
  state->connection_status = ssl.connection_status;
  state->security_bits = ssl.security_bits;
  state->sct_verify_statuses.clear();
  state->sct_verify_statuses.insert(state->sct_verify_statuses.end(),
                                    ssl.num_unknown_scts,
                                    net::ct::SCT_STATUS_LOG_UNKNOWN);
  state->sct_verify_statuses.insert(state->sct_verify_statuses.end(),
                                    ssl.num_invalid_scts,
                                    net::ct::SCT_STATUS_INVALID);
  state->sct_verify_statuses.insert(state->sct_verify_statuses.end(),
                                    ssl.num_valid_scts, net::ct::SCT_STATUS_OK);
  state->displayed_mixed_content =
      (ssl.content_status & content::SSLStatus::DISPLAYED_INSECURE_CONTENT)
          ? true
          : false;
  state->ran_mixed_content =
      (ssl.content_status & content::SSLStatus::RAN_INSECURE_CONTENT) ? true
                                                                      : false;
}

}  // namespace atom
