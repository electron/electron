// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/login_handler.h"

#include "atom/browser/browser.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"

using content::BrowserThread;

namespace atom {

LoginHandler::LoginHandler(
    net::AuthChallengeInfo* auth_info,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    const GURL& url,
    const base::Callback<void(const base::Optional<net::AuthCredentials>&)>&
        auth_required_callback)
    : handled_auth_(false),
      auth_info_(auth_info),
      web_contents_getter_(web_contents_getter),
      auth_required_callback_(auth_required_callback) {
  // Fill request details on IO thread.
  // TODO(deepak1556): Fill in method and referrer details to
  // avoid breaking the app login event.
  std::unique_ptr<base::DictionaryValue> request_details(
      new base::DictionaryValue);
  request_details->SetKey("url", base::Value(url.spec()));

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&Browser::RequestLogin, base::Unretained(Browser::Get()),
                     base::RetainedRef(WrapRefCounted(this)),
                     std::move(request_details)));
}

LoginHandler::~LoginHandler() {}

content::WebContents* LoginHandler::GetWebContents() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return web_contents_getter_.Run();
}

void LoginHandler::Login(const base::string16& username,
                         const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (TestAndSetAuthHandled())
    return;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&LoginHandler::DoLogin, this, username, password));
}

void LoginHandler::CancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (TestAndSetAuthHandled())
    return;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(&LoginHandler::DoCancelAuth, this));
}

void LoginHandler::OnRequestCancelled() {
  TestAndSetAuthHandled();
  auth_required_callback_.Reset();
}

// Marks authentication as handled and returns the previous handled state.
bool LoginHandler::TestAndSetAuthHandled() {
  base::AutoLock lock(handled_auth_lock_);
  bool was_handled = handled_auth_;
  handled_auth_ = true;
  return was_handled;
}

void LoginHandler::DoCancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!auth_required_callback_.is_null())
    std::move(auth_required_callback_).Run(base::nullopt);
}

void LoginHandler::DoLogin(const base::string16& username,
                           const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!auth_required_callback_.is_null()) {
    std::move(auth_required_callback_)
        .Run(net::AuthCredentials(username, password));
  }
}

}  // namespace atom
