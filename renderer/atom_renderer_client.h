// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDERER_CLIENT_
#define ATOM_RENDERER_ATOM_RENDERER_CLIENT_

#include "content/public/renderer/content_renderer_client.h"

namespace atom {

class AtomRendererBindings;
class NodeBindings;

class AtomRendererClient : public content::ContentRendererClient {
 public:
  AtomRendererClient();
  virtual ~AtomRendererClient();

  AtomRendererBindings* atom_bindings() const { return atom_bindings_.get(); }
  NodeBindings* node_bindings() const { return node_bindings_.get(); }

 private:
  virtual void RenderThreadStarted() OVERRIDE;
  virtual void RenderViewCreated(content::RenderView*) OVERRIDE;

  scoped_ptr<AtomRendererBindings> atom_bindings_;
  scoped_ptr<NodeBindings> node_bindings_;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDERER_CLIENT_
