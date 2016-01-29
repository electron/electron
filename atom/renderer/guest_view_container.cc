// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/guest_view_container.h"

#include <map>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/message_loop/message_loop.h"
#include "ui/gfx/geometry/size.h"

namespace atom {

namespace {

using GuestViewContainerMap = std::map<int, GuestViewContainer*>;
static base::LazyInstance<GuestViewContainerMap> g_guest_view_container_map =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

GuestViewContainer::GuestViewContainer(content::RenderFrame* render_frame)
    : render_frame_(render_frame),
      weak_ptr_factory_(this) {
}

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

  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(element_resize_callback_, new_size));
}

base::WeakPtr<content::BrowserPluginDelegate> GuestViewContainer::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace atom
