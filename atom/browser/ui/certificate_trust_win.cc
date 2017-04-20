// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/certificate_trust.h"

#include <windows.h>  // windows.h must be included first
#include <wincrypt.h>

namespace certificate_trust {

void ShowCertificateTrust(atom::NativeWindow* parent_window,
                          const scoped_refptr<net::X509Certificate>& cert,
                          const std::string& message,
                          const ShowTrustCallback& callback) {

    BOOL result = false;
    HCERTSTORE hCertStore = NULL;
    PCCERT_CONTEXT pCertContext = cert->CreateOSCertChainForCert();

    // opening the Trusted Root Certificate store for the current user
    hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"Root");

    // NOTE: this is a blocking call which displays a prompt to the user to
    //       confirm they trust this certificate
    result = CertAddCertificateContextToStore(hCertStore, pCertContext, CERT_STORE_ADD_REPLACE_EXISTING, NULL);

    // close certificate store
    CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);

    // free certificate
    CertFreeCertificateContext(pCertContext);

    &callback;
}

}  // namespace certificate_trust
