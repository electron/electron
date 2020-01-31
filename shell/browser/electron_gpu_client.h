// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_
#define SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_

#include "content/public/gpu/content_gpu_client.h"

namespace electron {

class ElectronGpuClient : public content::ContentGpuClient {
 public:
  ElectronGpuClient();

  // content::ContentGpuClient:
  void PreCreateMessageLoop() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronGpuClient);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_
