// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_view_proxy.h"

namespace atom {

OffscreenViewProxy::OffscreenViewProxy(views::View* view) : view_(view) {
  view_bitmap_.reset(new SkBitmap);
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
  if (GetBoundsScaled().width() == bitmap.width() &&
      GetBoundsScaled().height() == bitmap.height() && observer_) {
    view_bitmap_.reset(new SkBitmap(bitmap));
    observer_->OnProxyViewPaint(GetBoundsScaled());
  }
}

const gfx::Rect& OffscreenViewProxy::GetBounds() {
  return view_bounds_;
}

gfx::Rect OffscreenViewProxy::GetBoundsScaled() {
  return gfx::Rect(std::floor(view_bounds_.x() * scale_factor_),
                   std::floor(view_bounds_.y() * scale_factor_),
                   std::floor(view_bounds_.width() * scale_factor_),
                   std::floor(view_bounds_.height() * scale_factor_));
}

void OffscreenViewProxy::SetBounds(const gfx::Rect& bounds) {
  view_bounds_ = bounds;
}

float OffscreenViewProxy::GetScaleFactor() {
  return scale_factor_;
}

void OffscreenViewProxy::SetScaleFactor(float scale) {
  scale_factor_ = scale;
}

void OffscreenViewProxy::OnEvent(ui::Event* event) {
  if (view_) {
    view_->OnEvent(event);
  }
}

}  // namespace atom
