// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
#define ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_

#include <map>
#include <memory>
#include <string>

#include "net/cert/cert_verifier.h"

namespace brightray {

class RequireCTDelegate;

}  // namespace brightray

namespace atom {

class CertVerifierRequest;

struct VerifyRequestParams {
  std::string hostname;
  std::string default_result;
  int error_code;
  scoped_refptr<net::X509Certificate> certificate;

  VerifyRequestParams();
  VerifyRequestParams(const VerifyRequestParams&);
  ~VerifyRequestParams();
};

class AtomCertVerifier : public net::CertVerifier {
 public:
  explicit AtomCertVerifier(brightray::RequireCTDelegate* ct_delegate);
  ~AtomCertVerifier() override;

  using VerifyProc = base::Callback<void(const VerifyRequestParams& request,
                                         net::CompletionOnceCallback)>;

  void SetVerifyProc(const VerifyProc& proc);

  const VerifyProc verify_proc() const { return verify_proc_; }
  brightray::RequireCTDelegate* ct_delegate() const { return ct_delegate_; }
  net::CertVerifier* default_verifier() const {
    return default_cert_verifier_.get();
  }

 protected:
  // net::CertVerifier:
  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             net::CompletionOnceCallback callback,
             std::unique_ptr<Request>* out_req,
             const net::NetLogWithSource& net_log) override;
  bool SupportsOCSPStapling() override;

 private:
  friend class CertVerifierRequest;

  void RemoveRequest(const RequestParams& params);
  CertVerifierRequest* FindRequest(const RequestParams& params);

  std::map<RequestParams, CertVerifierRequest*> inflight_requests_;
  VerifyProc verify_proc_;
  std::unique_ptr<net::CertVerifier> default_cert_verifier_;
  brightray::RequireCTDelegate* ct_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AtomCertVerifier);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
