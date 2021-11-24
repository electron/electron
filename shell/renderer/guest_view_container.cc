// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/guest_view_container.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/render_frame.h"
#include "ui/gfx/geometry/size.h"

namespace electron {

namespace {

using GuestViewContainerMap = std::map<int, GuestViewContainer*>;
static base::LazyInstance<GuestViewContainerMap>::DestructorAtExit
    g_guest_view_container_map = LAZY_INSTANCE_INITIALIZER;

}  // namespace

GuestViewContainer::GuestViewContainer(content::RenderFrame* render_frame) {}

GuestViewContainer::~GuestViewContainer() {
  if (element_instance_id_ > 0)
    g_guest_view_container_map.Get().erase(element_instance_id_);
}

// static
GuestViewContainer* GuestViewContainer::FromID(int element_instance_id) {
  GuestViewContainerMap* guest_view_containers =
      g_guest_view_container_map.Pointer();
  auto it = guest_view_containers->find(element_instance_id);
  return it == guest_view_containers->end() ? nullptr : it->second;
}

void GuestViewContainer::RegisterElementResizeCallback(
    const ResizeCallback& callback) {
  element_resize_callback_ = callback;
}

void GuestViewContainer::SetElementInstanceID(int element_instance_id) {
  element_instance_id_ = element_instance_id;
  g_guest_view_container_map.Get().insert(
      std::make_pair(element_instance_id, this));
}

void GuestViewContainer::DidResizeElement(const gfx::Size& new_size) {
  if (element_resize_callback_.is_null())
    return;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(element_resize_callback_, new_size));
}

}  // namespace electron
