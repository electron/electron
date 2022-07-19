// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/services/node/node_service.h"

namespace electron {

NodeService::NodeService(
    mojo::PendingReceiver<node::mojom::NodeService> receiver) {
  if (receiver.is_valid())
    receiver_.Bind(std::move(receiver));
}

NodeService::~NodeService() = default;

}  // namespace electron
