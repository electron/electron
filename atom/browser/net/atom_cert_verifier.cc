// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_cert_verifier.h"

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace atom {

void AtomCertVerifier::CertVerifyRequest::ContinueWithResult(int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (handled_)
    return;

  handled_ = true;

  if (result != net::ERR_IO_PENDING) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(callback_, result));
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CertVerifyRequest::DelegateToDefaultVerifier, this));
}

void AtomCertVerifier::CertVerifyRequest::DelegateToDefaultVerifier() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int result = cert_verifier_->default_cert_verifier_->Verify(
      cert_.get(), hostname_, ocsp_response_, flags_, crl_set_.get(),
      verify_result_, callback_, out_req_, net_log_);
  if (result != net::ERR_IO_PENDING)
    callback_.Run(result);
}

AtomCertVerifier::AtomCertVerifier()
    : delegate_(nullptr) {
  default_cert_verifier_.reset(net::CertVerifier::CreateDefault());
}

AtomCertVerifier::~AtomCertVerifier() {
}

int AtomCertVerifier::Verify(
    net::X509Certificate* cert,
    const std::string& hostname,
    const std::string& ocsp_response,
    int flags,
    net::CRLSet* crl_set,
    net::CertVerifyResult* verify_result,
    const net::CompletionCallback& callback,
    scoped_ptr<Request>* out_req,
    const net::BoundNetLog& net_log) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (callback.is_null() || !verify_result || hostname.empty() || !delegate_)
    return net::ERR_INVALID_ARGUMENT;

  CertVerifyRequest* request = new CertVerifyRequest(
      this, cert, hostname, ocsp_response, flags, crl_set, verify_result,
      callback, out_req, net_log);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&Delegate::RequestCertVerification,
                                     base::Unretained(delegate_),
                                     make_scoped_refptr(request)));
  return net::ERR_IO_PENDING;
}

bool AtomCertVerifier::SupportsOCSPStapling() {
  return true;
}

}  // namespace atom
