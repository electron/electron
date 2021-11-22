// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_

#include "content/public/gpu/content_gpu_client.h"

namespace electron {

class ElectronGpuClient : public content::ContentGpuClient {
 public:
  ElectronGpuClient();

  // disable copy
  ElectronGpuClient(const ElectronGpuClient&) = delete;
  ElectronGpuClient& operator=(const ElectronGpuClient&) = delete;

  // content::ContentGpuClient:
  void PreCreateMessageLoop() override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_
