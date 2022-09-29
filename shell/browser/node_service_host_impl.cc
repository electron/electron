// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/node_service_host_impl.h"

#include <utility>

#include "content/browser/utility_process_host.h"
#include "content/public/browser/child_process_data.h"
#include "shell/browser/api/electron_api_utility_process.h"

namespace electron {

NodeServiceHostImpl::NodeServiceHostImpl(
    content::UtilityProcessHost* utility_process_host)
    : utility_process_host_(utility_process_host) {}

NodeServiceHostImpl::~NodeServiceHostImpl() = default;

void NodeServiceHostImpl::ReceivePostMessage(
    blink::TransferableMessage message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (utility_process_host_) {
    base::ProcessId pid = utility_process_host_->GetData().GetProcess().Pid();
    auto utility_process_wrapper =
        api::UtilityProcessWrapper::FromProcessId(pid);
    if (utility_process_wrapper)
      utility_process_wrapper->ReceivePostMessage(std::move(message));
  }
}

}  // namespace electron
