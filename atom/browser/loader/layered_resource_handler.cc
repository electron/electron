// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/loader/layered_resource_handler.h"

namespace atom {

LayeredResourceHandler::LayeredResourceHandler(
    net::URLRequest* request,
    std::unique_ptr<content::ResourceHandler> next_handler,
    Delegate* delegate)
    : content::LayeredResourceHandler(request, std::move(next_handler)),
      delegate_(delegate) {}

LayeredResourceHandler::~LayeredResourceHandler() {}

bool LayeredResourceHandler::OnResponseStarted(
    content::ResourceResponse* response,
    bool* defer) {
  if (delegate_)
    delegate_->OnResponseStarted(response);
  return next_handler_->OnResponseStarted(response, defer);
}

}  // namespace atom
