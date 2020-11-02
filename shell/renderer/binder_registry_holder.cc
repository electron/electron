// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/binder_registry_holder.h"

namespace electron {

BinderRegistryHolder::BinderRegistryHolder(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

BinderRegistryHolder::~BinderRegistryHolder() {}

void BinderRegistryHolder::OnInterfaceRequestForFrame(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void BinderRegistryHolder::OnDestruct() {
  delete this;
}

}  // namespace electron
