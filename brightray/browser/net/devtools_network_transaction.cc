// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/net/devtools_network_transaction.h"

#include "browser/net/devtools_network_controller.h"
#include "browser/net/devtools_network_interceptor.h"

#include "net/base/net_errors.h"
#include "net/base/upload_progress.h"
#include "net/http/http_network_transaction.h"
#include "net/http/http_request_info.h"
#include "net/socket/connection_attempts.h"

namespace brightray {

namespace {

const char kDevToolsEmulateNetworkConditionsClientId[] =
    "X-DevTools-Emulate-Network-Conditions-Client-Id";

}  // namespace

DevToolsNetworkTransaction::DevToolsNetworkTransaction(
    DevToolsNetworkController* controller,
    scoped_ptr<net::HttpTransaction> transaction)
    : controller_(controller),
      transaction_(transaction.Pass()),
      request_(nullptr),
      failed_(false),
      throttled_byte_count_(0),
      callback_type_(NONE) {
  proxy_callback_ = base::Bind(&DevToolsNetworkTransaction::OnCallback,
                                base::Unretained(this));
}

DevToolsNetworkTransaction::~DevToolsNetworkTransaction() {
  if (interceptor_)
    interceptor_->RemoveTransaction(this);
}

void DevToolsNetworkTransaction::ProcessRequest() {
  DCHECK(request_);

  bool has_devtools_client_id = request_->extra_headers.HasHeader(
      kDevToolsEmulateNetworkConditionsClientId);
  if (!has_devtools_client_id)
    return;

  custom_request_.reset(new net::HttpRequestInfo(*request_));
  custom_request_->extra_headers.GetHeader(
      kDevToolsEmulateNetworkConditionsClientId, &client_id_);
  custom_request_->extra_headers.RemoveHeader(
      kDevToolsEmulateNetworkConditionsClientId);

  request_ = custom_request_.get();
}

void DevToolsNetworkTransaction::Fail() {
  DCHECK(request_);
  DCHECK(!failed_);

  failed_ = true;
  transaction_->SetBeforeNetworkStartCallback(
      BeforeNetworkStartCallback());

  if (callback_.is_null())
    return;

  net::CompletionCallback original_callback = callback_;
  callback_.Reset();
  callback_type_ = NONE;
  original_callback.Run(net::ERR_INTERNET_DISCONNECTED);
}

void DevToolsNetworkTransaction::DecreaseThrottledByteCount(
    int64_t delta) {
  throttled_byte_count_ -= delta;
}

int DevToolsNetworkTransaction::Start(
    const net::HttpRequestInfo* request,
    const net::CompletionCallback& callback,
    const net::BoundNetLog& net_log) {
  DCHECK(request);

  request_ = request;
  interceptor_ = controller_->GetInterceptor(this);
  interceptor_->AddTransaction(this);

  if (interceptor_->ShouldFail(this)) {
    failed_ = true;
    transaction_->SetBeforeNetworkStartCallback(BeforeNetworkStartCallback());
    return net::ERR_INTERNET_DISCONNECTED;
  }
  int rv = transaction_->Start(request_, proxy_callback_, net_log);
  return SetupCallback(callback, rv, START);
}

int DevToolsNetworkTransaction::RestartIgnoringLastError(
    const net::CompletionCallback& callback) {
  if (failed_)
    return net::ERR_INTERNET_DISCONNECTED;
  int rv = transaction_->RestartIgnoringLastError(proxy_callback_);
  return SetupCallback(callback, rv, RESTART_IGNORING_LAST_ERROR);
}

int DevToolsNetworkTransaction::RestartWithCertificate(
    net::X509Certificate* client_certificate,
    const net::CompletionCallback& callback) {
  if (failed_)
    return net::ERR_INTERNET_DISCONNECTED;
  int rv = transaction_->RestartWithCertificate(client_certificate, proxy_callback_);
  return SetupCallback(callback, rv, RESTART_WITH_CERTIFICATE);
}

int DevToolsNetworkTransaction::RestartWithAuth(
    const net::AuthCredentials& credentials,
    const net::CompletionCallback& callback) {
  if (failed_)
    return net::ERR_INTERNET_DISCONNECTED;
  int rv = transaction_->RestartWithAuth(credentials, proxy_callback_);
  return SetupCallback(callback, rv, RESTART_WITH_AUTH);
}

bool DevToolsNetworkTransaction::IsReadyToRestartForAuth() {
  return transaction_->IsReadyToRestartForAuth();
}

int DevToolsNetworkTransaction::Read(
    net::IOBuffer* buffer,
    int length,
    const net::CompletionCallback& callback) {
  if (failed_)
    return net::ERR_INTERNET_DISCONNECTED;
  int rv = transaction_->Read(buffer, length, proxy_callback_);
  return SetupCallback(callback, rv, READ);
}

void DevToolsNetworkTransaction::StopCaching() {
  transaction_->StopCaching();
}

bool DevToolsNetworkTransaction::GetFullRequestHeaders(
    net::HttpRequestHeaders* headers) const {
  return transaction_->GetFullRequestHeaders(headers);
}

int64_t DevToolsNetworkTransaction::GetTotalReceivedBytes() const {
  return transaction_->GetTotalReceivedBytes();
}

void DevToolsNetworkTransaction::DoneReading() {
  transaction_->DoneReading();
}

const net::HttpResponseInfo*
DevToolsNetworkTransaction::GetResponseInfo() const {
  return transaction_->GetResponseInfo();
}

net::LoadState DevToolsNetworkTransaction::GetLoadState() const {
  return transaction_->GetLoadState();
}

net::UploadProgress DevToolsNetworkTransaction::GetUploadProgress() const {
  return transaction_->GetUploadProgress();
}

void DevToolsNetworkTransaction::SetQuicServerInfo(
    net::QuicServerInfo* info) {
  transaction_->SetQuicServerInfo(info);
}

bool DevToolsNetworkTransaction::GetLoadTimingInfo(
    net::LoadTimingInfo* info) const {
  return transaction_->GetLoadTimingInfo(info);
}

void DevToolsNetworkTransaction::SetPriority(net::RequestPriority priority) {
  transaction_->SetPriority(priority);
}

void DevToolsNetworkTransaction::SetWebSocketHandshakeStreamCreateHelper(
    net::WebSocketHandshakeStreamBase::CreateHelper* helper) {
  transaction_->SetWebSocketHandshakeStreamCreateHelper(helper);
}

void DevToolsNetworkTransaction::SetBeforeNetworkStartCallback(
    const BeforeNetworkStartCallback& callback) {
  transaction_->SetBeforeNetworkStartCallback(callback);
}

void DevToolsNetworkTransaction::SetBeforeProxyHeadersSentCallback(
    const BeforeProxyHeadersSentCallback& callback) {
  transaction_->SetBeforeProxyHeadersSentCallback(callback);
}

int DevToolsNetworkTransaction::ResumeNetworkStart() {
  if (failed_)
    return net::ERR_INTERNET_DISCONNECTED;
  return transaction_->ResumeNetworkStart();
}

void DevToolsNetworkTransaction::GetConnectionAttempts(
    net::ConnectionAttempts* out) const {
  transaction_->GetConnectionAttempts(out);
}

void DevToolsNetworkTransaction::OnCallback(int rv) {
  if (failed_ || callback_.is_null())
    return;

  if (callback_type_ == START || callback_type_ == READ) {
    if (interceptor_ && interceptor_->ShouldThrottle(this)) {
      Throttle(rv);
      return;
    }
  }

  net::CompletionCallback original_callback = callback_;
  callback_.Reset();
  callback_type_ = NONE;
  original_callback.Run(rv);
}

int DevToolsNetworkTransaction::SetupCallback(
    net::CompletionCallback callback,
    int result,
    CallbackType callback_type) {
  DCHECK(callback_type_ == NONE);

  if (result == net::ERR_IO_PENDING) {
    callback_type_ = callback_type;
    callback_ = callback;
    return result;
  }

  if (!interceptor_ || !interceptor_->ShouldThrottle(this))
    return result;

  // Only START and READ operation throttling is supported.
  if (callback_type != START && callback_type != READ)
    return result;

  // In case of error |throttled_byte_count_| is unknown.
  if (result < 0)
    return result;

  // URLRequestJob relies on synchronous end-of-stream notification.
  if (callback_type == READ && result == 0)
    return result;

  callback_type_ = callback_type;
  callback_ = callback;
  Throttle(result);
  return net::ERR_IO_PENDING;
}

void DevToolsNetworkTransaction::Throttle(int result) {
  throttled_result_ = result;

  if (callback_type_ == START)
    throttled_byte_count_ += transaction_->GetTotalReceivedBytes();
  if (result > 0)
    throttled_byte_count_ += result;

  if (interceptor_)
    interceptor_->ThrottleTransaction(this, callback_type_ == START);
}

void DevToolsNetworkTransaction::FireThrottledCallback() {
  DCHECK(!callback_.is_null());
  DCHECK(callback_type_ == READ || callback_type_ == START);

  net::CompletionCallback original_callback = callback_;
  callback_.Reset();
  callback_type_ = NONE;
  original_callback.Run(throttled_result_);
}

}  // namespace brightray
