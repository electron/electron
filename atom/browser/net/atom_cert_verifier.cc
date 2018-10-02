// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_cert_verifier.h"

#include <utility>

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "base/containers/linked_list.h"
#include "base/memory/weak_ptr.h"
#include "brightray/browser/net/require_ct_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace atom {

VerifyRequestParams::VerifyRequestParams() = default;
VerifyRequestParams::~VerifyRequestParams() = default;
VerifyRequestParams::VerifyRequestParams(const VerifyRequestParams&) = default;

namespace {

class Response : public base::LinkNode<Response> {
 public:
  Response(net::CertVerifyResult* verify_result,
           net::CompletionOnceCallback callback)
      : verify_result_(verify_result), callback_(std::move(callback)) {}
  net::CertVerifyResult* verify_result() { return verify_result_; }
  net::CompletionOnceCallback callback() { return std::move(callback_); }

 private:
  net::CertVerifyResult* verify_result_;
  net::CompletionOnceCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(Response);
};

}  // namespace

class CertVerifierRequest : public AtomCertVerifier::Request {
 public:
  CertVerifierRequest(const AtomCertVerifier::RequestParams& params,
                      AtomCertVerifier* cert_verifier)
      : params_(params),
        cert_verifier_(cert_verifier),
        weak_ptr_factory_(this) {}

  ~CertVerifierRequest() override {
    cert_verifier_->RemoveRequest(params_);
    default_verifier_request_.reset();
    while (!response_list_.empty() && !first_response_) {
      base::LinkNode<Response>* response_node = response_list_.head();
      response_node->RemoveFromList();
      Response* response = response_node->value();
      RunResponse(response);
    }
    cert_verifier_ = nullptr;
    weak_ptr_factory_.InvalidateWeakPtrs();
  }

  void RunResponse(Response* response) {
    if (custom_response_ == net::ERR_ABORTED) {
      *(response->verify_result()) = result_;
      response->callback().Run(error_);
    } else {
      response->verify_result()->Reset();
      response->verify_result()->verified_cert = params_.certificate();
      cert_verifier_->ct_delegate()->AddCTExcludedHost(params_.hostname());
      response->callback().Run(custom_response_);
    }
    delete response;
  }

  void Start(net::CRLSet* crl_set, const net::NetLogWithSource& net_log) {
    int error = cert_verifier_->default_verifier()->Verify(
        params_, crl_set, &result_,
        base::Bind(&CertVerifierRequest::OnDefaultVerificationDone,
                   weak_ptr_factory_.GetWeakPtr()),
        &default_verifier_request_, net_log);
    if (error != net::ERR_IO_PENDING)
      OnDefaultVerificationDone(error);
  }

  void OnDefaultVerificationDone(int error) {
    error_ = error;
    auto request = std::make_unique<VerifyRequestParams>();
    request->hostname = params_.hostname();
    request->default_result = net::ErrorToString(error);
    request->error_code = error;
    request->certificate = params_.certificate();
    auto response_callback = base::Bind(&CertVerifierRequest::OnResponseInUI,
                                        weak_ptr_factory_.GetWeakPtr());
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&CertVerifierRequest::OnVerifyRequestInUI,
                       cert_verifier_->verify_proc(), std::move(request),
                       response_callback));
  }

  static void OnVerifyRequestInUI(
      const AtomCertVerifier::VerifyProc& verify_proc,
      std::unique_ptr<VerifyRequestParams> request,
      const base::Callback<void(int)>& response_callback) {
    verify_proc.Run(*(request.get()), response_callback);
  }

  static void OnResponseInUI(base::WeakPtr<CertVerifierRequest> self,
                             int result) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&CertVerifierRequest::NotifyResponseInIO, self, result));
  }

  void NotifyResponseInIO(int result) {
    custom_response_ = result;
    first_response_ = false;
    // Responding to first request in the list will initiate destruction of
    // the class, respond to others in the list inside destructor.
    base::LinkNode<Response>* response_node = response_list_.head();
    response_node->RemoveFromList();
    Response* response = response_node->value();
    RunResponse(response);
  }

  void AddResponseListener(net::CertVerifyResult* verify_result,
                           net::CompletionOnceCallback callback) {
    response_list_.Append(new Response(verify_result, std::move(callback)));
  }

  const AtomCertVerifier::RequestParams& params() const { return params_; }

 private:
  using ResponseList = base::LinkedList<Response>;

  const AtomCertVerifier::RequestParams params_;
  AtomCertVerifier* cert_verifier_;
  int error_ = net::ERR_IO_PENDING;
  int custom_response_ = net::ERR_IO_PENDING;
  bool first_response_ = true;
  ResponseList response_list_;
  net::CertVerifyResult result_;
  std::unique_ptr<AtomCertVerifier::Request> default_verifier_request_;
  base::WeakPtrFactory<CertVerifierRequest> weak_ptr_factory_;
};

AtomCertVerifier::AtomCertVerifier(brightray::RequireCTDelegate* ct_delegate)
    : default_cert_verifier_(net::CertVerifier::CreateDefault()),
      ct_delegate_(ct_delegate) {}

AtomCertVerifier::~AtomCertVerifier() {}

void AtomCertVerifier::SetVerifyProc(const VerifyProc& proc) {
  verify_proc_ = proc;
}

int AtomCertVerifier::Verify(const RequestParams& params,
                             net::CRLSet* crl_set,
                             net::CertVerifyResult* verify_result,
                             net::CompletionOnceCallback callback,
                             std::unique_ptr<Request>* out_req,
                             const net::NetLogWithSource& net_log) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (verify_proc_.is_null()) {
    ct_delegate_->ClearCTExcludedHostsList();
    return default_cert_verifier_->Verify(
        params, crl_set, verify_result, std::move(callback), out_req, net_log);
  } else {
    CertVerifierRequest* request = FindRequest(params);
    if (!request) {
      out_req->reset();
      auto new_request = std::make_unique<CertVerifierRequest>(params, this);
      new_request->Start(crl_set, net_log);
      request = new_request.get();
      *out_req = std::move(new_request);
      inflight_requests_[params] = request;
    }
    request->AddResponseListener(verify_result, std::move(callback));

    return net::ERR_IO_PENDING;
  }
}

void AtomCertVerifier::RemoveRequest(const RequestParams& params) {
  auto it = inflight_requests_.find(params);
  if (it != inflight_requests_.end())
    inflight_requests_.erase(it);
}

CertVerifierRequest* AtomCertVerifier::FindRequest(
    const RequestParams& params) {
  auto it = inflight_requests_.find(params);
  if (it != inflight_requests_.end())
    return it->second;
  return nullptr;
}

}  // namespace atom
