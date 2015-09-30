// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_resource_dispatcher_host_delegate.h"

#include "atom/common/platform_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_view_host.h"
#include "net/base/escape.h"

using content::BrowserThread;

namespace atom {

namespace {

void HandleExternalProtocolInUI(const GURL& url,
                                int render_process_id,
                                int render_view_id) {
  auto web_contents = content::WebContents::FromRenderViewHost(
      content::RenderViewHost::FromID(render_process_id, render_view_id));
  if (!web_contents)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  platform_util::OpenExternal(escaped_url);
}

}  // namespace

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {
}

AtomResourceDispatcherHostDelegate::~AtomResourceDispatcherHostDelegate() {
}

bool AtomResourceDispatcherHostDelegate::HandleExternalProtocol(
    const GURL& url,
    int render_process_id,
    int render_view_id,
    bool is_main_frame,
    ui::PageTransition transition,
    bool has_user_gesture) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&HandleExternalProtocolInUI,
                 url,
                 render_process_id,
                 render_view_id));
  return true;
}

}  // namespace atom
