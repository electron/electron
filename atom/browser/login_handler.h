// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_LOGIN_HANDLER_H_
#define ATOM_BROWSER_LOGIN_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/resource_dispatcher_host_login_delegate.h"

namespace net {
class AuthChallengeInfo;
class URLRequest;
}

namespace atom {

class LoginHandler : public content::ResourceDispatcherHostLoginDelegate {
 public:
  LoginHandler(net::AuthChallengeInfo* auth_info, net::URLRequest* request);

 protected:
  ~LoginHandler() override;

  // content::ResourceDispatcherHostLoginDelegate:
  void OnRequestCancelled() override;

 private:
  // Who/where/what asked for the authentication.
  scoped_refptr<net::AuthChallengeInfo> auth_info_;

  // The request that wants login data.
  // This should only be accessed on the IO loop.
  net::URLRequest* request_;

  base::WeakPtrFactory<LoginHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(LoginHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_LOGIN_HANDLER_H_
