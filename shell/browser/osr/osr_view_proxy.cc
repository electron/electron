// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/osr/osr_view_proxy.h"

#include <memory>

namespace electron {

OffscreenViewProxy::OffscreenViewProxy(views::View* view) : view_(view) {
  view_bitmap_ = std::make_unique<SkBitmap>();
}

OffscreenViewProxy::~OffscreenViewProxy() {
  if (observer_) {
    observer_->ProxyViewDestroyed(this);
  }
}

void OffscreenViewProxy::SetObserver(OffscreenViewProxyObserver* observer) {
  if (observer_) {
    observer_->ProxyViewDestroyed(this);
  }
  observer_ = observer;
}

void OffscreenViewProxy::RemoveObserver() {
  observer_ = nullptr;
}

const SkBitmap* OffscreenViewProxy::GetBitmap() const {
  return view_bitmap_.get();
}

void OffscreenViewProxy::SetBitmap(const SkBitmap& bitmap) {
  if (view_bounds_.width() == bitmap.width() &&
      view_bounds_.height() == bitmap.height() && observer_) {
    view_bitmap_ = std::make_unique<SkBitmap>(bitmap);
    observer_->OnProxyViewPaint(view_bounds_);
  }
}

const gfx::Rect& OffscreenViewProxy::GetBounds() {
  return view_bounds_;
}

void OffscreenViewProxy::SetBounds(const gfx::Rect& bounds) {
  view_bounds_ = bounds;
}

void OffscreenViewProxy::OnEvent(ui::Event* event) {
  if (view_) {
    view_->OnEvent(event);
  }
}

}  // namespace electron
