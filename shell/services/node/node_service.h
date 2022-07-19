// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_SERVICES_NODE_NODE_SERVICE_H_
#define ELECTRON_SHELL_SERVICES_NODE_NODE_SERVICE_H_

#include "mojo/public/cpp/bindings/receiver.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"

namespace electron {

class NodeService : public node::mojom::NodeService {
 public:
  explicit NodeService(
      mojo::PendingReceiver<node::mojom::NodeService> receiver);

  NodeService(const NodeService&) = delete;
  NodeService& operator=(const NodeService&) = delete;

  ~NodeService() override;

 private:
  mojo::Receiver<node::mojom::NodeService> receiver_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_SERVICES_NODE_NODE_SERVICE_H_
