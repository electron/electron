// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_H_
#define ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_H_

#include "common/api/atom_bindings.h"

#include "base/string16.h"

namespace base {
class ListValue;
}

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

  // Dispatch messages from browser.
  void OnBrowserMessage(const string16& channel,
                        const base::ListValue& args);

 private:
  content::RenderView* render_view_;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererBindings);
};

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_BINDINGS_H_
