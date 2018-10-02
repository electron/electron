// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/login_handler.h"

#include <memory>
#include <utility>

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"

using content::BrowserThread;

namespace atom {

LoginHandler::LoginHandler(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    net::NetworkDelegate::AuthCallback callback,
    net::AuthCredentials* credentials,
    const content::ResourceRequestInfo* resource_request_info)
    : credentials_(credentials),
      auth_info_(auth_info),
      auth_callback_(std::move(callback)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<base::DictionaryValue> request_details(
      new base::DictionaryValue);
  FillRequestDetails(request_details.get(), request);

  web_contents_getter_ =
      resource_request_info->GetWebContentsGetterForRequest();

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&Browser::RequestLogin, base::Unretained(Browser::Get()),
                     base::RetainedRef(this), std::move(request_details)));
}

LoginHandler::~LoginHandler() {}

void LoginHandler::Login(const base::string16& username,
                         const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LoginHandler::DoLogin, weak_factory_.GetWeakPtr(),
                     username, password));
}

void LoginHandler::CancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LoginHandler::DoCancelAuth, weak_factory_.GetWeakPtr()));
}

void LoginHandler::NotifyRequestDestroyed() {
  auth_callback_.Reset();
  credentials_ = nullptr;
  weak_factory_.InvalidateWeakPtrs();
}

content::WebContents* LoginHandler::GetWebContents() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return web_contents_getter_.Run();
}

void LoginHandler::DoCancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!auth_callback_.is_null())
    std::move(auth_callback_)
        .Run(net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_CANCEL_AUTH);
}

void LoginHandler::DoLogin(const base::string16& username,
                           const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!auth_callback_.is_null()) {
    credentials_->Set(username, password);
    std::move(auth_callback_)
        .Run(net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_SET_AUTH);
  }
}

}  // namespace atom
