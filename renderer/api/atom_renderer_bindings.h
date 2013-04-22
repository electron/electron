// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_
#define ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_

#include "common/api/atom_bindings.h"

namespace atom {

class AtomRendererBindings : public AtomBindings {
 public:
  AtomRendererBindings();
  virtual ~AtomRendererBindings();

  // Call BindTo for process object of the frame.
  void BindToFrame(WebKit::WebFrame* frame);

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomRendererBindings);
};

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_BINDINGS_
