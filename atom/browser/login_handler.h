// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_LOGIN_HANDLER_H_
#define ATOM_BROWSER_LOGIN_HANDLER_H_

#include "base/strings/string16.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/resource_dispatcher_host_login_delegate.h"

namespace content {
class WebContents;
}

namespace net {
class AuthChallengeInfo;
class URLRequest;
}

namespace atom {

// Handles the HTTP basic auth, must be created on IO thread.
class LoginHandler : public content::ResourceDispatcherHostLoginDelegate {
 public:
  LoginHandler(net::AuthChallengeInfo* auth_info, net::URLRequest* request);

  // Returns the WebContents associated with the request, must be called on UI
  // thread.
  content::WebContents* GetWebContents() const;

  // The auth is cancelled, must be called on UI thread.
  void CancelAuth();

  // Login with |username| and |password|, must be called on UI thread.
  void Login(const base::string16& username, const base::string16& password);

  const net::AuthChallengeInfo* auth_info() const { return auth_info_.get(); }
  const net::URLRequest* request() const { return request_; }

 protected:
  ~LoginHandler() override;

  // content::ResourceDispatcherHostLoginDelegate:
  void OnRequestCancelled() override;

 private:
  // Must be called on IO thread.
  void DoCancelAuth();
  void DoLogin(const base::string16& username, const base::string16& password);

  // Marks authentication as handled and returns the previous handled
  // state.
  bool TestAndSetAuthHandled();

  // True if we've handled auth (Login or CancelAuth has been called).
  bool handled_auth_;
  mutable base::Lock handled_auth_lock_;

  // Who/where/what asked for the authentication.
  scoped_refptr<net::AuthChallengeInfo> auth_info_;

  // The request that wants login data.
  // This should only be accessed on the IO loop.
  net::URLRequest* request_;

  // Cached from the net::URLRequest, in case it goes NULL on us.
  int render_process_host_id_;
  int render_frame_id_;

  DISALLOW_COPY_AND_ASSIGN(LoginHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_LOGIN_HANDLER_H_
