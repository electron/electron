// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_LOGIN_HANDLER_H_
#define ATOM_BROWSER_LOGIN_HANDLER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/strings/string16.h"
#include "content/public/browser/resource_request_info.h"
#include "net/base/network_delegate.h"

namespace content {
class WebContents;
}

namespace atom {

// Handles the HTTP basic auth, must be created on IO thread.
class LoginHandler : public base::RefCountedThreadSafe<LoginHandler> {
 public:
  LoginHandler(net::URLRequest* request,
               const net::AuthChallengeInfo& auth_info,
               net::NetworkDelegate::AuthCallback callback,
               net::AuthCredentials* credentials,
               const content::ResourceRequestInfo* resource_request_info);

  // The auth is cancelled, must be called on UI thread.
  void CancelAuth();

  // The URLRequest associated with the auth is destroyed.
  void NotifyRequestDestroyed();

  // Login with |username| and |password|, must be called on UI thread.
  void Login(const base::string16& username, const base::string16& password);

  // Returns the WebContents associated with the request, must be called on UI
  // thread.
  content::WebContents* GetWebContents() const;

  const net::AuthChallengeInfo* auth_info() const { return &auth_info_; }

 private:
  friend class base::RefCountedThreadSafe<LoginHandler>;
  friend class base::DeleteHelper<LoginHandler>;

  ~LoginHandler();

  // Must be called on IO thread.
  void DoCancelAuth();
  void DoLogin(const base::string16& username, const base::string16& password);

  // Credentials to be used for the auth.
  net::AuthCredentials* credentials_;

  // Who/where/what asked for the authentication.
  const net::AuthChallengeInfo& auth_info_;

  // WebContents associated with the login request.
  content::ResourceRequestInfo::WebContentsGetter web_contents_getter_;

  // Called with preferred value of net::NetworkDelegate::AuthRequiredResponse.
  net::NetworkDelegate::AuthCallback auth_callback_;

  base::WeakPtrFactory<LoginHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LoginHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_LOGIN_HANDLER_H_
