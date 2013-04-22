// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_
#define ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_

#include "common/api/atom_bindings.h"

namespace content {
class RenderView;
}

namespace WebKit {
class WebFrame;
}

namespace atom {

class AtomRendererBindings : public AtomBindings {
 public:
  explicit AtomRendererBindings(content::RenderView* render_view);
  virtual ~AtomRendererBindings();

  // Call BindTo for process object of the frame.
  void BindToFrame(WebKit::WebFrame* frame);

  // Add process.send and make process.on accept IPC message.
  void AddIPCBindings(WebKit::WebFrame* frame);

 private:
  static v8::Handle<v8::Value> Send(const v8::Arguments &args);

  content::RenderView* render_view_;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererBindings);
};

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_BINDINGS_
