// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_GPU_CLIENT_H_
#define ATOM_BROWSER_ATOM_GPU_CLIENT_H_

#include "content/public/gpu/content_gpu_client.h"

namespace atom {

class AtomGpuClient : public content::ContentGpuClient {
 public:
  AtomGpuClient();

  // content::ContentGpuClient:
  void PreCreateMessageLoop() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomGpuClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_GPU_CLIENT_H_
