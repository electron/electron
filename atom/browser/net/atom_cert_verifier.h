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
  class CertVerifyRequest
      : public base::RefCountedThreadSafe<CertVerifyRequest> {
   public:
    CertVerifyRequest(AtomCertVerifier* cert_verifier,
                      net::X509Certificate* cert,
                      const std::string& hostname,
                      const std::string& ocsp_response,
                      int flags,
                      net::CRLSet* crl_set,
                      net::CertVerifyResult* verify_result,
                      const net::CompletionCallback& callback,
                      scoped_ptr<Request>* out_req,
                      const net::BoundNetLog& net_log)
        : cert_verifier_(cert_verifier),
          cert_(cert),
          hostname_(hostname),
          ocsp_response_(ocsp_response),
          flags_(flags),
          crl_set_(crl_set),
          verify_result_(verify_result),
          callback_(callback),
          out_req_(out_req),
          net_log_(net_log),
          handled_(false) {
    }

    void ContinueWithResult(int result);

    const std::string& hostname() const { return hostname_; }
    net::X509Certificate* cert() const { return cert_.get(); }

   private:
    friend class base::RefCountedThreadSafe<CertVerifyRequest>;
    ~CertVerifyRequest() {}

    void DelegateToDefaultVerifier();

    AtomCertVerifier* cert_verifier_;

    scoped_refptr<net::X509Certificate> cert_;
    std::string hostname_;
    std::string ocsp_response_;
    int flags_;
    scoped_refptr<net::CRLSet> crl_set_;
    net::CertVerifyResult* verify_result_;
    net::CompletionCallback callback_;
    scoped_ptr<Request>* out_req_;
    const net::BoundNetLog& net_log_;

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

  Delegate* delegate_;
  scoped_ptr<net::CertVerifier> default_cert_verifier_;

  DISALLOW_COPY_AND_ASSIGN(AtomCertVerifier);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
