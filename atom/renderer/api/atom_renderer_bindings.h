// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_H_
#define ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_H_

#include "atom/common/api/atom_bindings.h"

#include "base/strings/string16.h"

namespace base {
class ListValue;
}

namespace content {
class RenderView;
}

namespace blink {
class WebFrame;
}

namespace atom {

class AtomRendererBindings : public AtomBindings {
 public:
  AtomRendererBindings();
  virtual ~AtomRendererBindings();

  // Call BindTo for process object of the frame.
  void BindToFrame(blink::WebFrame* frame);

  // Dispatch messages from browser.
  void OnBrowserMessage(content::RenderView* render_view,
                        const base::string16& channel,
                        const base::ListValue& args);

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomRendererBindings);
};

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_RENDERER_BINDINGS_H_
