// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_PLUGIN_INFO_HOST_IMPL_H_
#define SHELL_BROWSER_ELECTRON_PLUGIN_INFO_HOST_IMPL_H_

#include <string>

#include "shell/common/plugin.mojom.h"

class GURL;

namespace url {
class Origin;
}

namespace electron {

// Implements ElectronPluginInfoHost interface.
class ElectronPluginInfoHostImpl : public mojom::ElectronPluginInfoHost {
 public:
  ElectronPluginInfoHostImpl();

  ElectronPluginInfoHostImpl(const ElectronPluginInfoHostImpl&) = delete;
  ElectronPluginInfoHostImpl& operator=(const ElectronPluginInfoHostImpl&) =
      delete;

  ~ElectronPluginInfoHostImpl() override;

  // mojom::ElectronPluginInfoHost
  void GetPluginInfo(const GURL& url,
                     const url::Origin& origin,
                     const std::string& mime_type,
                     GetPluginInfoCallback callback) override;
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_PLUGIN_INFO_HOST_IMPL_H_
