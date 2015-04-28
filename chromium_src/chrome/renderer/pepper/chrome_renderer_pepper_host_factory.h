// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PEPPER_CHROME_RENDERER_PEPPER_HOST_FACTORY_H_
#define CHROME_RENDERER_PEPPER_CHROME_RENDERER_PEPPER_HOST_FACTORY_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ppapi/host/host_factory.h"

namespace content {
class RendererPpapiHost;
}

class ChromeRendererPepperHostFactory : public ppapi::host::HostFactory {
 public:
  explicit ChromeRendererPepperHostFactory(content::RendererPpapiHost* host);
  ~ChromeRendererPepperHostFactory() override;

  // HostFactory.
  scoped_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // Not owned by this object.
  content::RendererPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(ChromeRendererPepperHostFactory);
};

#endif  // CHROME_RENDERER_PEPPER_CHROME_RENDERER_PEPPER_HOST_FACTORY_H_
