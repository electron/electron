// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_SERVICES_NODE_NODE_SERVICE_H_
#define ELECTRON_SHELL_SERVICES_NODE_NODE_SERVICE_H_

#include <memory>
#include <string>

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"

namespace electron {

class ElectronBindings;
class JavascriptEnvironment;
class NodeBindings;
class NodeEnvironment;

class NodeService : public node::mojom::NodeService {
 public:
  explicit NodeService(
      mojo::PendingReceiver<node::mojom::NodeService> receiver);

  NodeService(const NodeService&) = delete;
  NodeService& operator=(const NodeService&) = delete;

  ~NodeService() override;

  // mojom::NodeService implementation:
  void Initialize(node::mojom::NodeServiceParamsPtr params) override;
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message) override;

 private:
  bool initialized_ = false;

  std::unique_ptr<JavascriptEnvironment> js_env_;
  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<ElectronBindings> electron_bindings_;
  std::unique_ptr<NodeEnvironment> node_env_;

  mojo::Receiver<node::mojom::NodeService> receiver_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_SERVICES_NODE_NODE_SERVICE_H_
