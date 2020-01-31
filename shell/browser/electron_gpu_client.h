// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ATOM_GPU_CLIENT_H_
#define SHELL_BROWSER_ATOM_GPU_CLIENT_H_

#include "content/public/gpu/content_gpu_client.h"

namespace electron {

class AtomGpuClient : public content::ContentGpuClient {
 public:
  AtomGpuClient();

  // content::ContentGpuClient:
  void PreCreateMessageLoop() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomGpuClient);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ATOM_GPU_CLIENT_H_
