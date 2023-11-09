// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/certificate_trust.h"

#include <windows.h>  // windows.h must be included first

#include <wincrypt.h>

#include "net/cert/cert_database.h"
#include "net/cert/x509_util_win.h"
#include "shell/browser/javascript_environment.h"

namespace certificate_trust {

// Add the provided certificate to the Trusted Root Certificate Authorities
// store for the current user.
//
// This requires prompting the user to confirm they trust the certificate.
BOOL AddToTrustedRootStore(const PCCERT_CONTEXT cert_context,
                           const scoped_refptr<net::X509Certificate>& cert) {
  auto* root_cert_store = CertOpenStore(
      CERT_STORE_PROV_SYSTEM, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"Root");

  if (root_cert_store == nullptr) {
    return false;
  }

  auto result = CertAddCertificateContextToStore(
      root_cert_store, cert_context, CERT_STORE_ADD_REPLACE_EXISTING, nullptr);

  if (result) {
    // force Chromium to reload it's database for this certificate
    auto* cert_db = net::CertDatabase::GetInstance();
    cert_db->NotifyObserversTrustStoreChanged();
  }

  CertCloseStore(root_cert_store, CERT_CLOSE_STORE_FORCE_FLAG);

  return result;
}

CERT_CHAIN_PARA GetCertificateChainParameters() {
  CERT_ENHKEY_USAGE enhkey_usage;
  enhkey_usage.cUsageIdentifier = 0;
  enhkey_usage.rgpszUsageIdentifier = nullptr;

  CERT_USAGE_MATCH cert_usage;
  // ensure the rules are applied to the entire chain
  cert_usage.dwType = USAGE_MATCH_TYPE_AND;
  cert_usage.Usage = enhkey_usage;

  CERT_CHAIN_PARA params = {sizeof(CERT_CHAIN_PARA)};
  params.RequestedUsage = cert_usage;

  return params;
}

v8::Local<v8::Promise> ShowCertificateTrust(
    electron::NativeWindow* parent_window,
    const scoped_refptr<net::X509Certificate>& cert,
    const std::string& message) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  PCCERT_CHAIN_CONTEXT chain_context;
  auto cert_context = net::x509_util::CreateCertContextWithChain(cert.get());
  auto params = GetCertificateChainParameters();

  if (CertGetCertificateChain(nullptr, cert_context.get(), nullptr, nullptr,
                              &params, 0, nullptr, &chain_context)) {
    auto error_status = chain_context->TrustStatus.dwErrorStatus;
    if (error_status == CERT_TRUST_IS_SELF_SIGNED ||
        error_status == CERT_TRUST_IS_UNTRUSTED_ROOT) {
      // these are the only scenarios we're interested in supporting
      AddToTrustedRootStore(cert_context.get(), cert);
    }

    CertFreeCertificateChain(chain_context);
  }

  promise.Resolve();
  return handle;
}

}  // namespace certificate_trust
