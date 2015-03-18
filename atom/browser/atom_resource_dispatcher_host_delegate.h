// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include "base/compiler_specific.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace atom {

class AtomResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  AtomResourceDispatcherHostDelegate();

  // content::ResourceDispatcherHostDelegate:
  void OnResponseStarted(net::URLRequest* request,
                         content::ResourceContext* resource_context,
                         content::ResourceResponse* response,
                         IPC::Sender* sender) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomResourceDispatcherHostDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
