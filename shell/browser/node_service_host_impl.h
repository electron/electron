// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NODE_SERVICE_HOST_IMPL_H_
#define ELECTRON_SHELL_BROWSER_NODE_SERVICE_HOST_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"

namespace content {
class UtilityProcessHost;
}  // namespace content

namespace electron {

class NodeServiceHostImpl : public node::mojom::NodeServiceHost {
 public:
  NodeServiceHostImpl(content::UtilityProcessHost* utility_process_host);

  NodeServiceHostImpl(const NodeServiceHostImpl&) = delete;
  NodeServiceHostImpl& operator=(const NodeServiceHostImpl&) = delete;

  ~NodeServiceHostImpl() override;

  // node::mojom::NodeServiceHost:
  void ReceivePostMessage(blink::TransferableMessage message) override;

 private:
  raw_ptr<content::UtilityProcessHost> utility_process_host_;
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NODE_SERVICE_HOST_IMPL_H_
