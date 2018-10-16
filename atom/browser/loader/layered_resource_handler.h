// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_LOADER_LAYERED_RESOURCE_HANDLER_H_
#define ATOM_BROWSER_LOADER_LAYERED_RESOURCE_HANDLER_H_

#include <memory>

#include "content/browser/loader/layered_resource_handler.h"
#include "services/network/public/cpp/resource_response.h"

namespace atom {

// Resource handler that notifies on various stages of a resource request.
class LayeredResourceHandler : public content::LayeredResourceHandler {
 public:
  class Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    virtual void OnResponseStarted(network::ResourceResponse* response) = 0;
  };

  LayeredResourceHandler(net::URLRequest* request,
                         std::unique_ptr<content::ResourceHandler> next_handler,
                         Delegate* delegate);
  ~LayeredResourceHandler() override;

  // content::LayeredResourceHandler:
  void OnResponseStarted(
      network::ResourceResponse* response,
      std::unique_ptr<content::ResourceController> controller) override;

 private:
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(LayeredResourceHandler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_LOADER_LAYERED_RESOURCE_HANDLER_H_
