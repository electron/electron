// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_ELECTRON_RENDERER_PEPPER_HOST_FACTORY_H_
#define SHELL_RENDERER_ELECTRON_RENDERER_PEPPER_HOST_FACTORY_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ppapi/host/host_factory.h"

namespace content {
class RendererPpapiHost;
}

class ElectronRendererPepperHostFactory : public ppapi::host::HostFactory {
 public:
  explicit ElectronRendererPepperHostFactory(content::RendererPpapiHost* host);
  ~ElectronRendererPepperHostFactory() override;

  // HostFactory.
  std::unique_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // Not owned by this object.
  content::RendererPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRendererPepperHostFactory);
};

#endif  // SHELL_RENDERER_ELECTRON_RENDERER_PEPPER_HOST_FACTORY_H_
