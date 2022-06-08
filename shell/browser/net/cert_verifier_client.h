// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_CERT_VERIFIER_CLIENT_H_
#define ELECTRON_SHELL_BROWSER_NET_CERT_VERIFIER_CLIENT_H_

#include <string>

#include "net/cert/x509_certificate.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace electron {

struct VerifyRequestParams {
  std::string hostname;
  std::string default_result;
  int error_code;
  scoped_refptr<net::X509Certificate> certificate;
  scoped_refptr<net::X509Certificate> validated_certificate;
  bool is_issued_by_known_root;

  VerifyRequestParams();
  VerifyRequestParams(const VerifyRequestParams&);
  ~VerifyRequestParams();
};

class CertVerifierClient : public network::mojom::CertVerifierClient {
 public:
  using CertVerifyProc =
      base::RepeatingCallback<void(const VerifyRequestParams& request,
                                   base::OnceCallback<void(int)>)>;

  explicit CertVerifierClient(CertVerifyProc proc);
  ~CertVerifierClient() override;

  // network::mojom::CertVerifierClient
  void Verify(int default_error,
              const net::CertVerifyResult& default_result,
              const scoped_refptr<net::X509Certificate>& certificate,
              const std::string& hostname,
              int flags,
              const absl::optional<std::string>& ocsp_response,
              VerifyCallback callback) override;

 private:
  CertVerifyProc cert_verify_proc_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_CERT_VERIFIER_CLIENT_H_
