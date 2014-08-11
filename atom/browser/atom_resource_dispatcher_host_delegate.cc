// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "base/logging.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace atom {

namespace {

const char* kDisableXFrameOptions = "disable-x-frame-options";

}  // namespace

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {
}

AtomResourceDispatcherHostDelegate::~AtomResourceDispatcherHostDelegate() {
}

void AtomResourceDispatcherHostDelegate::OnResponseStarted(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::ResourceResponse* response,
    IPC::Sender* sender) {
  // Check if frame's name contains "disable-x-frame-options"
  int p, f;
  if (!content::ResourceRequestInfo::GetRenderFrameForRequest(request, &p, &f))
    return;
  content::RenderFrameHost* frame = content::RenderFrameHost::FromID(p, f);
  if (!frame)
    return;
  if (frame->GetFrameName().find(kDisableXFrameOptions) == std::string::npos)
    return;

  // Remove the "X-Frame-Options" from response headers.
  net::HttpResponseHeaders* response_headers = request->response_headers();
  if (response_headers && response_headers->HasHeader("x-frame-options"))
    response_headers->RemoveHeader("x-frame-options");
}

}  // namespace atom
