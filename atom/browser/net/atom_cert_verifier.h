// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
#define ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "net/cert/cert_verifier.h"

namespace atom {

class AtomCertVerifier : public net::CertVerifier {
 public:
  struct VerifyArgs {
    scoped_refptr<net::X509Certificate> cert;
    const std::string& hostname;
    net::CompletionCallback callback;
  };

  class CertVerifyRequest
      : public base::RefCountedThreadSafe<CertVerifyRequest> {
   public:
    CertVerifyRequest(AtomCertVerifier* cert_verifier,
                      int result,
                      const VerifyArgs& args)
        : cert_verifier_(cert_verifier),
          result_(result),
          args_(args),
          handled_(false) {
    }

    void ContinueWithResult(int result);

    const VerifyArgs& args() const { return args_; }

   private:
    friend class base::RefCountedThreadSafe<CertVerifyRequest>;
    ~CertVerifyRequest();

    AtomCertVerifier* cert_verifier_;
    int result_;
    VerifyArgs args_;
    bool handled_;

    DISALLOW_COPY_AND_ASSIGN(CertVerifyRequest);
  };

  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called on UI thread.
    virtual void RequestCertVerification(
        const scoped_refptr<CertVerifyRequest>& request) {}
  };

  AtomCertVerifier();
  virtual ~AtomCertVerifier();

  void SetDelegate(Delegate* delegate) {
    delegate_ = delegate;
  }

 protected:
  // net::CertVerifier:
  int Verify(net::X509Certificate* cert,
             const std::string& hostname,
             const std::string& ocsp_response,
             int flags,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             const net::CompletionCallback& callback,
             scoped_ptr<Request>* out_req,
             const net::BoundNetLog& net_log) override;
  bool SupportsOCSPStapling() override;

 private:
  friend class CertVerifyRequest;

  void VerifyCertificateFromDelegate(const VerifyArgs& args, int result);
  void OnDefaultVerificationResult(const VerifyArgs& args, int result);

  Delegate* delegate_;
  scoped_ptr<net::CertVerifier> default_cert_verifier_;

  DISALLOW_COPY_AND_ASSIGN(AtomCertVerifier);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
