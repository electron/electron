// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDERER_CLIENT_H_
#define ATOM_RENDERER_ATOM_RENDERER_CLIENT_H_

#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"

namespace node {
class Environment;
}

namespace atom {

class AtomRendererBindings;
class NodeBindings;

class AtomRendererClient : public content::ContentRendererClient {
 public:
  AtomRendererClient();
  virtual ~AtomRendererClient();

  bool IsNodeBindingEnabled(WebKit::WebFrame* frame = NULL);

  AtomRendererBindings* atom_bindings() const { return atom_bindings_.get(); }

 private:
  enum NodeIntegration {
    ALL,
    EXCEPT_IFRAME,
    MANUAL_ENABLE_IFRAME,
    DISABLE,
  };

  virtual void RenderThreadStarted() OVERRIDE;
  virtual void RenderViewCreated(content::RenderView*) OVERRIDE;
  virtual void DidCreateScriptContext(WebKit::WebFrame* frame,
                                      v8::Handle<v8::Context> context,
                                      int extension_group,
                                      int world_id) OVERRIDE;
  virtual void WillReleaseScriptContext(WebKit::WebFrame* frame,
                                        v8::Handle<v8::Context>,
                                        int world_id) OVERRIDE;
  virtual bool ShouldFork(WebKit::WebFrame* frame,
                          const GURL& url,
                          const std::string& http_method,
                          bool is_initial_navigation,
                          bool is_server_redirect,
                          bool* send_referrer);

  std::vector<node::Environment*> web_page_envs_;

  scoped_ptr<NodeBindings> node_bindings_;
  scoped_ptr<AtomRendererBindings> atom_bindings_;

  // The level of node integration we should support.
  NodeIntegration node_integration_;

  // The main frame.
  WebKit::WebFrame* main_frame_;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDERER_CLIENT_H_
