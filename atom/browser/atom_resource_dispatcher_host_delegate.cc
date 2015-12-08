// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "atom/browser/login_handler.h"
#include "atom/common/platform_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace atom {

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {
}

bool AtomResourceDispatcherHostDelegate::HandleExternalProtocol(
    const GURL& url,
    int render_process_id,
    int render_view_id,
    bool is_main_frame,
    ui::PageTransition transition,
    bool has_user_gesture) {
  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(base::IgnoreResult(platform_util::OpenExternal), escaped_url));
  return true;
}

content::ResourceDispatcherHostLoginDelegate*
AtomResourceDispatcherHostDelegate::CreateLoginDelegate(
    net::AuthChallengeInfo* auth_info,
    net::URLRequest* request) {
  return new LoginHandler(auth_info, request);
}

}  // namespace atom
