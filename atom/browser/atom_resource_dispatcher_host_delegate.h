// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include <string>

#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace atom {

class AtomResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  AtomResourceDispatcherHostDelegate();

  // content::ResourceDispatcherHostDelegate:
  bool HandleExternalProtocol(const GURL& url,
                              content::ResourceRequestInfo* info) override;
  content::ResourceDispatcherHostLoginDelegate* CreateLoginDelegate(
      net::AuthChallengeInfo* auth_info,
      net::URLRequest* request) override;
  std::unique_ptr<net::ClientCertStore> CreateClientCertStore(
      content::ResourceContext* resource_context) override;
  bool ShouldInterceptResourceAsStream(net::URLRequest* request,
                                       const base::FilePath& plugin_path,
                                       const std::string& mime_type,
                                       GURL* origin,
                                       std::string* payload) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomResourceDispatcherHostDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
