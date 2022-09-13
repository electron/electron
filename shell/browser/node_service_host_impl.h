// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NODE_SERVICE_HOST_IMPL_H_
#define ELECTRON_SHELL_BROWSER_NODE_SERVICE_HOST_IMPL_H_

#include "base/sequence_checker.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"

namespace electron {

class NodeServiceHostImpl : public node::mojom::NodeServiceHost {
 public:
  NodeServiceHostImpl();

  NodeServiceHostImpl(const NodeServiceHostImpl&) = delete;
  NodeServiceHostImpl& operator=(const NodeServiceHostImpl&) = delete;

  ~NodeServiceHostImpl() override;

  void BindReceiver(
      mojo::PendingReceiver<node::mojom::NodeServiceHost> receiver);

  // node::mojom::NodeServiceHost:
  void ReceivePostMessage(base::ProcessId pid,
                          blink::TransferableMessage message) override;

 private:
  mojo::ReceiverSet<node::mojom::NodeServiceHost> receivers_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NODE_SERVICE_HOST_IMPL_H_
