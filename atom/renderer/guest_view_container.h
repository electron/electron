// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_GUEST_VIEW_CONTAINER_H_
#define ATOM_RENDERER_GUEST_VIEW_CONTAINER_H_

#include "content/public/renderer/browser_plugin_delegate.h"
#include "v8/include/v8.h"

namespace atom {

class GuestViewContainer : public content::BrowserPluginDelegate {
 public:
  explicit GuestViewContainer(content::RenderFrame* render_frame);
  ~GuestViewContainer() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GuestViewContainer);
};

}  // namespace atom

#endif  // ATOM_RENDERER_GUEST_VIEW_CONTAINER_H_
