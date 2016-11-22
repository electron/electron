// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_cert_verifier.h"

#include "atom/browser/browser.h"
#include "atom/browser/net/atom_ct_delegate.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace atom {

namespace {

void OnResult(
    net::CertVerifyResult* verify_result,
    const net::CompletionCallback& callback,
    bool result) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(callback, result ? net::OK : net::ERR_FAILED));
}

}  // namespace

AtomCertVerifier::AtomCertVerifier(AtomCTDelegate* ct_delegate)
    : default_cert_verifier_(net::CertVerifier::CreateDefault()),
      ct_delegate_(ct_delegate) {}

AtomCertVerifier::~AtomCertVerifier() {}

void AtomCertVerifier::SetVerifyProc(const VerifyProc& proc) {
  verify_proc_ = proc;
}

int AtomCertVerifier::Verify(
    const RequestParams& params,
    net::CRLSet* crl_set,
    net::CertVerifyResult* verify_result,
    const net::CompletionCallback& callback,
    std::unique_ptr<Request>* out_req,
    const net::BoundNetLog& net_log) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (verify_proc_.is_null()) {
    ct_delegate_->ClearCTExcludedHostsList();
    return default_cert_verifier_->Verify(
        params, crl_set, verify_result, callback, out_req, net_log);
  }

  verify_result->Reset();
  verify_result->verified_cert = params.certificate();
  ct_delegate_->AddCTExcludedHost(params.hostname());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(verify_proc_, params.hostname(), params.certificate(),
                 base::Bind(OnResult, verify_result, callback)));
  return net::ERR_IO_PENDING;
}

bool AtomCertVerifier::SupportsOCSPStapling() {
  return true;
}

}  // namespace atom
