// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/node_service_host_impl.h"

#include <utility>

#include "shell/browser/api/electron_api_utility_process.h"

namespace electron {

NodeServiceHostImpl::NodeServiceHostImpl() = default;

NodeServiceHostImpl::~NodeServiceHostImpl() = default;

void NodeServiceHostImpl::BindReceiver(
    mojo::PendingReceiver<node::mojom::NodeServiceHost> receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receivers_.Add(this, std::move(receiver));
}

void NodeServiceHostImpl::ReceivePostMessage(
    base::ProcessId pid,
    blink::TransferableMessage message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* utility_process_wrapper = GetAllUtilityProcessWrappers().Lookup(pid);
  if (utility_process_wrapper)
    utility_process_wrapper->ReceivePostMessage(std::move(message));
}

}  // namespace electron
