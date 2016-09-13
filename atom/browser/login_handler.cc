// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/login_handler.h"

#include "atom/browser/browser.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
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
    : handled_auth_(false),
      auth_info_(auth_info),
      request_(request),
      render_process_host_id_(0),
      render_frame_id_(0) {
  content::ResourceRequestInfo::ForRequest(request_)->GetAssociatedRenderFrame(
      &render_process_host_id_,  &render_frame_id_);

  // Fill request details on IO thread.
  std::unique_ptr<base::DictionaryValue> request_details(
      new base::DictionaryValue);
  FillRequestDetails(request_details.get(), request_);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&Browser::RequestLogin,
                 base::Unretained(Browser::Get()),
                 base::RetainedRef(make_scoped_refptr(this)),
                 base::Passed(&request_details)));
}

LoginHandler::~LoginHandler() {
}

content::WebContents* LoginHandler::GetWebContents() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::RenderFrameHost* rfh = content::RenderFrameHost::FromID(
      render_process_host_id_, render_frame_id_);
  return content::WebContents::FromRenderFrameHost(rfh);
}

void LoginHandler::Login(const base::string16& username,
                         const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (TestAndSetAuthHandled())
    return;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&LoginHandler::DoLogin, this, username, password));
}

void LoginHandler::CancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (TestAndSetAuthHandled())
    return;
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&LoginHandler::DoCancelAuth, this));
}

void LoginHandler::OnRequestCancelled() {
  TestAndSetAuthHandled();
  request_ = nullptr;
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
