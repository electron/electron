// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_
#define ELECTRON_SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_

#include "base/callback.h"

namespace content {
class RenderFrame;
}

namespace gfx {
class Size;
}

namespace electron {

class GuestViewContainer {
 public:
  typedef base::RepeatingCallback<void(const gfx::Size&)> ResizeCallback;

  explicit GuestViewContainer(content::RenderFrame* render_frame);
  virtual ~GuestViewContainer();

  // disable copy
  GuestViewContainer(const GuestViewContainer&) = delete;
  GuestViewContainer& operator=(const GuestViewContainer&) = delete;

  static GuestViewContainer* FromID(int element_instance_id);

  void RegisterElementResizeCallback(const ResizeCallback& callback);
  void SetElementInstanceID(int element_instance_id);
  void DidResizeElement(const gfx::Size& new_size);

 private:
  int element_instance_id_;

  ResizeCallback element_resize_callback_;

  base::WeakPtrFactory<GuestViewContainer> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_
