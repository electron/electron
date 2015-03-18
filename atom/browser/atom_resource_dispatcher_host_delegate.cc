// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include <string>

#include "base/logging.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace atom {

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {
}

void AtomResourceDispatcherHostDelegate::OnResponseStarted(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::ResourceResponse* response,
    IPC::Sender* sender) {
  // Remove the "X-Frame-Options" from response headers for devtools.
  if (request->url().SchemeIs("chrome-devtools")) {
    net::HttpResponseHeaders* response_headers = request->response_headers();
    if (response_headers && response_headers->HasHeader("x-frame-options"))
      response_headers->RemoveHeader("x-frame-options");
  }
}

}  // namespace atom
