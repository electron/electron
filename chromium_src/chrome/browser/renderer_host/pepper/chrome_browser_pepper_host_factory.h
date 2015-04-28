// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_CHROME_BROWSER_PEPPER_HOST_FACTORY_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_CHROME_BROWSER_PEPPER_HOST_FACTORY_H_

#include "base/compiler_specific.h"
#include "ppapi/host/host_factory.h"

namespace content {
class BrowserPpapiHost;
}  // namespace content

namespace chrome {

class ChromeBrowserPepperHostFactory : public ppapi::host::HostFactory {
 public:
  // Non-owning pointer to the filter must outlive this class.
  explicit ChromeBrowserPepperHostFactory(content::BrowserPpapiHost* host);
  ~ChromeBrowserPepperHostFactory() override;

  scoped_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // Non-owning pointer.
  content::BrowserPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserPepperHostFactory);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_CHROME_BROWSER_PEPPER_HOST_FACTORY_H_
