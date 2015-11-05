// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_cert_verifier.h"

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace atom {

namespace {

void RunResult(const net::CompletionCallback& callback, bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int result = net::OK;
  if (!success)
    result = net::ERR_FAILED;

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(callback, result));
}

}  // namespace

AtomCertVerifier::AtomCertVerifier() {
  Browser::Get()->AddObserver(this);
  default_cert_verifier_.reset(net::CertVerifier::CreateDefault());
}

AtomCertVerifier::~AtomCertVerifier() {
  Browser::Get()->RemoveObserver(this);
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

  if (callback.is_null() || !verify_result || hostname.empty())
    return net::ERR_INVALID_ARGUMENT;

  if (!handler_.is_null()) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(handler_, hostname,
                                       make_scoped_refptr(cert),
                                       base::Bind(&RunResult, callback)));
    return net::ERR_IO_PENDING;
  }

  return default_cert_verifier_->Verify(cert, hostname, ocsp_response,
                                        flags, crl_set, verify_result,
                                        callback, out_req, net_log);
}

bool AtomCertVerifier::SupportsOCSPStapling() {
  if (handler_.is_null())
    return default_cert_verifier_->SupportsOCSPStapling();
  return false;
}

void AtomCertVerifier::OnSetCertificateVerifier(
    const CertificateVerifier& handler) {
  handler_ = handler;
}

void AtomCertVerifier::OnRemoveCertificateVerifier() {
  handler_.Reset();
}

}  // namespace atom
