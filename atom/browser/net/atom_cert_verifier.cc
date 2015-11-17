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

AtomCertVerifier::CertVerifyRequest::~CertVerifyRequest() {
}

void AtomCertVerifier::CertVerifyRequest::ContinueWithResult(int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (handled_)
    return;

  handled_ = true;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(args_.callback,
                 result == net::ERR_IO_PENDING ? result_ : result));
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

  VerifyArgs args = { cert, hostname, callback };
  int result = default_cert_verifier_->Verify(
      cert, hostname, ocsp_response, flags, crl_set, verify_result,
      base::Bind(&AtomCertVerifier::OnDefaultVerificationResult,
                 base::Unretained(this), args),
      out_req, net_log);
  if (result != net::OK && result != net::ERR_IO_PENDING) {
    // The default verifier fails immediately.
    VerifyCertificateFromDelegate(args, result);
    return net::ERR_IO_PENDING;
  }

  return result;
}

bool AtomCertVerifier::SupportsOCSPStapling() {
  return true;
}

void AtomCertVerifier::VerifyCertificateFromDelegate(
    const VerifyArgs& args, int result) {
  CertVerifyRequest* request = new CertVerifyRequest(this, result, args);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&Delegate::RequestCertVerification,
                                     base::Unretained(delegate_),
                                     make_scoped_refptr(request)));
}

void AtomCertVerifier::OnDefaultVerificationResult(
    const VerifyArgs& args, int result) {
  if (result == net::OK) {
    args.callback.Run(result);
    return;
  }

  VerifyCertificateFromDelegate(args, result);
}

}  // namespace atom
