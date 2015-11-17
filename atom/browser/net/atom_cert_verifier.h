// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
#define ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_

#include <set>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "net/base/hash_value.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_certificate.h"
#include "net/log/net_log.h"

namespace atom {

class AtomCertVerifier : public net::CertVerifier {
 public:
  struct RequestParams {
    RequestParams(
        const net::SHA1HashValue cert_fingerprint,
        const net::SHA1HashValue ca_fingerprint,
        const std::string& hostname_arg,
        const std::string& ocsp_response,
        int flags);
    ~RequestParams() {}

    bool operator<(const RequestParams& other) const;

    std::string hostname;
    std::string ocsp_response;
    int flags;
    std::vector<net::SHA1HashValue> hash_values;
  };

  class CertVerifyRequest
      : public base::RefCountedThreadSafe<CertVerifyRequest> {
   public:
    CertVerifyRequest(
        AtomCertVerifier* cert_verifier,
        const RequestParams& key,
        scoped_refptr<net::X509Certificate> cert,
        scoped_refptr<net::CRLSet> crl_set,
        net::CertVerifyResult* verify_result,
        scoped_ptr<Request>* out_req,
        const net::BoundNetLog& net_log)
        : cert_verifier_(cert_verifier),
          key_(key),
          certificate_(cert),
          crl_set_(crl_set),
          verify_result_(verify_result),
          out_req_(out_req),
          net_log_(net_log),
          handled_(false) {
    }

    void RunResult(int result);
    void DelegateToDefaultVerifier();
    void ContinueWithResult(int result);

    void AddCompletionCallback(net::CompletionCallback callback) {
      callbacks_.push_back(callback);
    }

    const RequestParams key() const { return key_; }

    std::string hostname() const { return key_.hostname; }

    scoped_refptr<net::X509Certificate> certificate() const {
      return certificate_;
    }

   private:
    friend class base::RefCountedThreadSafe<CertVerifyRequest>;
    ~CertVerifyRequest() {}

    AtomCertVerifier* cert_verifier_;
    const RequestParams key_;

    scoped_refptr<net::X509Certificate> certificate_;
    scoped_refptr<net::CRLSet> crl_set_;
    net::CertVerifyResult* verify_result_;
    scoped_ptr<Request>* out_req_;
    const net::BoundNetLog net_log_;

    std::vector<net::CompletionCallback> callbacks_;
    bool handled_;

    DISALLOW_COPY_AND_ASSIGN(CertVerifyRequest);
  };

  class Delegate {
   public:
    Delegate() {}
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

  net::CertVerifier* default_cert_verifier() const {
    return default_cert_verifier_.get();
  }

 private:
  CertVerifyRequest* FindRequest(const RequestParams& key);
  void RemoveRequest(CertVerifyRequest* request);

  struct CertVerifyRequestToRequestParamsComparator {
    bool operator()(const scoped_refptr<CertVerifyRequest> request,
                    const RequestParams& key) const {
      return request->key() < key;
    }
  };

  struct CertVerifyRequestComparator {
    bool operator()(const scoped_refptr<CertVerifyRequest> req1,
                    const scoped_refptr<CertVerifyRequest> req2) const {
      return req1->key() < req2->key();
    }
  };

  using ActiveRequestSet =
      std::set<scoped_refptr<CertVerifyRequest>,
               CertVerifyRequestComparator>;
  ActiveRequestSet requests_;

  Delegate* delegate_;

  scoped_ptr<net::CertVerifier> default_cert_verifier_;

  DISALLOW_COPY_AND_ASSIGN(AtomCertVerifier);
};

}   // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_CERT_VERIFIER_H_
