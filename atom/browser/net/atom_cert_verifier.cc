// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_cert_verifier.h"

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "base/sha1.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"

using content::BrowserThread;

namespace atom {

AtomCertVerifier::RequestParams::RequestParams(
    const net::SHA1HashValue cert_fingerprint,
    const net::SHA1HashValue ca_fingerprint,
    const std::string& hostname_arg,
    const std::string& ocsp_response_arg,
    int flags_arg)
    : hostname(hostname_arg),
      ocsp_response(ocsp_response_arg),
      flags(flags_arg) {
  hash_values.reserve(3);
  net::SHA1HashValue ocsp_hash;
  base::SHA1HashBytes(
      reinterpret_cast<const unsigned char*>(ocsp_response.data()),
      ocsp_response.size(), ocsp_hash.data);
  hash_values.push_back(ocsp_hash);
  hash_values.push_back(cert_fingerprint);
  hash_values.push_back(ca_fingerprint);
}

bool AtomCertVerifier::RequestParams::operator<(
    const RequestParams& other) const {
  if (flags != other.flags)
    return flags < other.flags;
  if (hostname != other.hostname)
    return hostname < other.hostname;
  return std::lexicographical_compare(
      hash_values.begin(),
      hash_values.end(),
      other.hash_values.begin(),
      other.hash_values.end(),
      net::SHA1HashValueLessThan());
}

void AtomCertVerifier::CertVerifyRequest::RunResult(int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (auto& callback : callbacks_)
    callback.Run(result);
  cert_verifier_->RemoveRequest(this);
}

void AtomCertVerifier::CertVerifyRequest::DelegateToDefaultVerifier() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  int rv = cert_verifier_->default_cert_verifier()->Verify(
      certificate_.get(),
      key_.hostname,
      key_.ocsp_response,
      key_.flags,
      crl_set_.get(),
      verify_result_,
      base::Bind(&CertVerifyRequest::RunResult, this),
      out_req_,
      net_log_);

  if (rv != net::ERR_IO_PENDING)
    RunResult(rv);
}

void AtomCertVerifier::CertVerifyRequest::ContinueWithResult(int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (handled_)
    return;

  handled_ = true;

  if (result != net::ERR_IO_PENDING) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(&CertVerifyRequest::RunResult,
                                       this,
                                       result));
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CertVerifyRequest::DelegateToDefaultVerifier, this));
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

  const RequestParams key(cert->fingerprint(),
                          cert->ca_fingerprint(),
                          hostname,
                          ocsp_response,
                          flags);

  CertVerifyRequest* request = FindRequest(key);

  if (!request) {
    request = new CertVerifyRequest(this,
                                    key,
                                    cert,
                                    crl_set,
                                    verify_result,
                                    out_req,
                                    net_log);
    requests_.insert(make_scoped_refptr(request));

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::Bind(&Delegate::RequestCertVerification,
                                       base::Unretained(delegate_),
                                       make_scoped_refptr(request)));
  }

  request->AddCompletionCallback(callback);

  return net::ERR_IO_PENDING;
}

bool AtomCertVerifier::SupportsOCSPStapling() {
  return true;
}

AtomCertVerifier::CertVerifyRequest* AtomCertVerifier::FindRequest(
    const RequestParams& key) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto it = std::lower_bound(requests_.begin(),
                             requests_.end(),
                             key,
                             CertVerifyRequestToRequestParamsComparator());
  if (it != requests_.end() && !(key < (*it)->key()))
    return (*it).get();
  return nullptr;
}

void AtomCertVerifier::RemoveRequest(CertVerifyRequest* request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  bool erased = requests_.erase(request) == 1;
  DCHECK(erased);
}

}  // namespace atom
