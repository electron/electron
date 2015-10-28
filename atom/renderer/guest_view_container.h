// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_GUEST_VIEW_CONTAINER_H_
#define ATOM_RENDERER_GUEST_VIEW_CONTAINER_H_

#include "base/callback.h"
#include "content/public/renderer/browser_plugin_delegate.h"

namespace gfx {
class Size;
}

namespace atom {

class GuestViewContainer : public content::BrowserPluginDelegate {
 public:
  typedef base::Callback<void(const gfx::Size&)> ResizeCallback;

  explicit GuestViewContainer(content::RenderFrame* render_frame);
  ~GuestViewContainer() override;

  static GuestViewContainer* FromID(int element_instance_id);

  void RegisterElementResizeCallback(const ResizeCallback& callback);

  // content::BrowserPluginDelegate:
  void SetElementInstanceID(int element_instance_id) final;
  void DidResizeElement(const gfx::Size& new_size) final;
  base::WeakPtr<BrowserPluginDelegate> GetWeakPtr() final;

 private:
  int element_instance_id_;
  content::RenderFrame* render_frame_;

  ResizeCallback element_resize_callback_;

  base::WeakPtrFactory<GuestViewContainer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GuestViewContainer);
};

}  // namespace atom

#endif  // ATOM_RENDERER_GUEST_VIEW_CONTAINER_H_
