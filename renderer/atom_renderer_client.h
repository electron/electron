// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDERER_CLIENT_
#define ATOM_RENDERER_ATOM_RENDERER_CLIENT_

#include "content/public/renderer/content_renderer_client.h"

namespace atom {

class AtomRendererClient : public content::ContentRendererClient {
public:
  AtomRendererClient();
  ~AtomRendererClient();

private:
  virtual void RenderViewCreated(content::RenderView*) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDERER_CLIENT_
