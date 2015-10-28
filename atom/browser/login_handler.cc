// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/login_handler.h"

#include "atom/browser/browser.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "net/base/auth.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;

namespace atom {

namespace {

// Helper to remove the ref from an net::URLRequest to the LoginHandler.
// Should only be called from the IO thread, since it accesses an
// net::URLRequest.
void ResetLoginHandlerForRequest(net::URLRequest* request) {
  content::ResourceDispatcherHost::Get()->ClearLoginDelegateForRequest(request);
}

}  // namespace

LoginHandler::LoginHandler(net::AuthChallengeInfo* auth_info,
                           net::URLRequest* request)
    : auth_info_(auth_info), request_(request) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&Browser::RequestLogin,
                                     base::Unretained(Browser::Get()),
                                     make_scoped_refptr(this)));
}

LoginHandler::~LoginHandler() {
}

void LoginHandler::Login(const base::string16& username,
                         const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&LoginHandler::DoLogin, this, username, password));
}

void LoginHandler::CancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&LoginHandler::DoCancelAuth, this));
}

void LoginHandler::OnRequestCancelled() {
  request_ = nullptr;
}

void LoginHandler::DoCancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (request_) {
    request_->CancelAuth();
    // Verify that CancelAuth doesn't destroy the request via our delegate.
    DCHECK(request_ != nullptr);
    ResetLoginHandlerForRequest(request_);
  }
}

void LoginHandler::DoLogin(const base::string16& username,
                           const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (request_) {
    request_->SetAuth(net::AuthCredentials(username, password));
    ResetLoginHandlerForRequest(request_);
  }
}

}  // namespace atom
