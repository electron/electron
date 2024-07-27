// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "net/cert/cert_verify_result.h"
#include "shell/browser/net/cert_verifier_client.h"

namespace electron {

VerifyRequestParams::VerifyRequestParams() = default;

VerifyRequestParams::~VerifyRequestParams() = default;

VerifyRequestParams::VerifyRequestParams(const VerifyRequestParams&) = default;

CertVerifierClient::CertVerifierClient(CertVerifyProc proc)
    : cert_verify_proc_(proc) {}

CertVerifierClient::~CertVerifierClient() = default;

void CertVerifierClient::Verify(
    int default_error,
    const net::CertVerifyResult& default_result,
    const scoped_refptr<net::X509Certificate>& certificate,
    const std::string& hostname,
    int flags,
    const std::optional<std::string>& ocsp_response,
    VerifyCallback callback) {
  VerifyRequestParams params;
  params.hostname = hostname;
  params.default_result = net::ErrorToString(default_error);
  params.error_code = default_error;
  params.certificate = certificate;
  params.validated_certificate = default_result.verified_cert;
  params.is_issued_by_known_root = default_result.is_issued_by_known_root;
  cert_verify_proc_.Run(
      params,
      base::BindOnce(
          [](VerifyCallback callback, const net::CertVerifyResult& result,
             int err) { std::move(callback).Run(err, result); },
          std::move(callback), default_result));
}

}  // namespace electron
