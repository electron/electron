// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace atom {

class AtomResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  AtomResourceDispatcherHostDelegate();

  // content::ResourceDispatcherHostDelegate:
  bool HandleExternalProtocol(
      const GURL& url,
      int child_id,
      const content::ResourceRequestInfo::WebContentsGetter&,
      bool is_main_frame,
      ui::PageTransition transition,
      bool has_user_gesture) override;
  content::ResourceDispatcherHostLoginDelegate* CreateLoginDelegate(
      net::AuthChallengeInfo* auth_info,
      net::URLRequest* request) override;
};

}  // namespace atom
