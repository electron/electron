// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/certificate_trust.h"

#include <wincrypt.h>
#include <windows.h>

#include "base/callback.h"
#include "net/cert/cert_database.h"

namespace certificate_trust {

BOOL AddCertificate(const HCERTSTORE certStore,
                    const PCCERT_CONTEXT certContext,
                    const scoped_refptr<net::X509Certificate>& cert) {
  auto result = CertAddCertificateContextToStore(
      certStore,
      certContext,
      CERT_STORE_ADD_REPLACE_EXISTING,
      NULL);

  if (result) {
    // force Chromium to reload it's database for this certificate
    auto cert_db = net::CertDatabase::GetInstance();
    cert_db->NotifyObserversCertDBChanged(cert.get());
  }

  return result;
}

// Add the provided certificate to the Trusted Root Certificate Authorities
// store for the current user.
//
// This requires prompting the user to confirm they trust the certificate.
BOOL AddToTrustedRootStore(const PCCERT_CONTEXT certContext,
                           const scoped_refptr<net::X509Certificate>& cert) {
  auto rootCertStore = CertOpenStore(
      CERT_STORE_PROV_SYSTEM,
      0,
      NULL,
      CERT_SYSTEM_STORE_CURRENT_USER,
      L"Root");

  if (rootCertStore == NULL) {
    return false;
  }

  auto result = AddCertificate(rootCertStore, certContext, cert);

  CertCloseStore(rootCertStore, CERT_CLOSE_STORE_FORCE_FLAG);

  return result;
}

// Add the provided certificate to the Personal
// certificate store for the current user.
BOOL AddToPersonalStore(const PCCERT_CONTEXT certContext,
                        const scoped_refptr<net::X509Certificate>& cert) {
  auto userCertStore = CertOpenStore(
      CERT_STORE_PROV_SYSTEM,
      0,
      NULL,
      CERT_SYSTEM_STORE_CURRENT_USER,
      L"My");

  if (userCertStore == NULL) {
      return false;
  }

  auto result = AddCertificate(userCertStore, certContext, cert);

  CertCloseStore(userCertStore, CERT_CLOSE_STORE_FORCE_FLAG);

  return result;
}

CERT_CHAIN_PARA GetCertificateChainParameters() {
  CERT_ENHKEY_USAGE enhkeyUsage;
  enhkeyUsage.cUsageIdentifier = 0;
  enhkeyUsage.rgpszUsageIdentifier = NULL;

  CERT_USAGE_MATCH CertUsage;
  // ensure the rules are applied to the entire chain
  CertUsage.dwType = USAGE_MATCH_TYPE_AND;
  CertUsage.Usage = enhkeyUsage;

  CERT_CHAIN_PARA params = { sizeof(CERT_CHAIN_PARA) };
  params.RequestedUsage = CertUsage;

  return params;
}

void ShowCertificateTrust(atom::NativeWindow* parent_window,
                          const scoped_refptr<net::X509Certificate>& cert,
                          const std::string& message,
                          const ShowTrustCallback& callback) {
  PCCERT_CHAIN_CONTEXT chainContext;

  auto pCertContext = cert->CreateOSCertChainForCert();

  auto params = GetCertificateChainParameters();

  if (CertGetCertificateChain(NULL,
                              pCertContext,
                              NULL,
                              NULL,
                              &params,
                              NULL,
                              NULL,
                              &chainContext)) {
    switch (chainContext->TrustStatus.dwErrorStatus) {
      case CERT_TRUST_NO_ERROR:
        AddToPersonalStore(pCertContext, cert);
        break;

      case CERT_TRUST_IS_UNTRUSTED_ROOT:
      case CERT_TRUST_IS_SELF_SIGNED:
        AddToTrustedRootStore(pCertContext, cert);
        break;

      default:
        // we can't handle other scenarios, giving up
        break;
    }

    CertFreeCertificateChain(chainContext);
  }

  CertFreeCertificateContext(pCertContext);

  callback.Run();
}

}  // namespace certificate_trust
