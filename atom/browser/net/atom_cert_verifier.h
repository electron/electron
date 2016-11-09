// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
#define ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_

#include <memory>
#include <string>

#include "net/cert/cert_verifier.h"

namespace atom {

class AtomCTDelegate;

class AtomCertVerifier : public net::CertVerifier {
 public:
  explicit AtomCertVerifier();
  virtual ~AtomCertVerifier();

  using VerifyProc =
      base::Callback<void(const std::string& hostname,
                          scoped_refptr<net::X509Certificate>,
                          const base::Callback<void(bool)>&,
                          bool default_result)>;

  void SetVerifyProc(const VerifyProc& proc);

 protected:
  // net::CertVerifier:
  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback& callback,
             std::unique_ptr<Request>* out_req,
             const net::BoundNetLog& net_log) override;
  bool SupportsOCSPStapling() override;

 private:
  VerifyProc verify_proc_;
  std::unique_ptr<net::CertVerifier> default_cert_verifier_;

  DISALLOW_COPY_AND_ASSIGN(AtomCertVerifier);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
