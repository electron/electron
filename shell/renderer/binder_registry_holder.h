// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_BINDER_REGISTRY_HOLDER_H_
#define SHELL_RENDERER_BINDER_REGISTRY_HOLDER_H_

#include <string>

#include "content/public/renderer/render_frame_observer.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace electron {

class BinderRegistryHolder : public content::RenderFrameObserver {
 public:
  explicit BinderRegistryHolder(content::RenderFrame* render_frame);
  ~BinderRegistryHolder() override;

  service_manager::BinderRegistry* registry() { return &registry_; }

 private:
  // RenderFrameObserver implementation.
  void OnInterfaceRequestForFrame(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;
  void OnDestruct() override;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(BinderRegistryHolder);
};

}  // namespace electron

#endif  // SHELL_RENDERER_BINDER_REGISTRY_HOLDER_H_
