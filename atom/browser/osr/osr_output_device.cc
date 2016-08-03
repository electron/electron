// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_output_device.h"

#include "third_party/skia/include/core/SkDevice.h"
#include "ui/gfx/skia_util.h"

namespace atom {

OffScreenOutputDevice::OffScreenOutputDevice(bool transparent,
                                             const OnPaintCallback& callback)
  : transparent_(transparent),
    callback_(callback),
    active_(false) {
  DCHECK(!callback_.is_null());
}

OffScreenOutputDevice::~OffScreenOutputDevice() {
}

void OffScreenOutputDevice::Resize(
    const gfx::Size& pixel_size, float scale_factor) {
  scale_factor_ = scale_factor;

  if (viewport_pixel_size_ == pixel_size) return;
  viewport_pixel_size_ = pixel_size;

  canvas_.reset();
  bitmap_.reset(new SkBitmap);
  bitmap_->allocN32Pixels(viewport_pixel_size_.width(),
                          viewport_pixel_size_.height(),
                          !transparent_);
  if (bitmap_->drawsNothing()) {
    NOTREACHED();
    bitmap_.reset();
    return;
  }

  if (transparent_)
    bitmap_->eraseARGB(0, 0, 0, 0);

  canvas_.reset(new SkCanvas(*bitmap_));
}

SkCanvas* OffScreenOutputDevice::BeginPaint(const gfx::Rect& damage_rect) {
  DCHECK(canvas_.get());
  DCHECK(bitmap_.get());

  damage_rect_ = damage_rect;

  return canvas_.get();
}

void OffScreenOutputDevice::EndPaint() {
  DCHECK(canvas_.get());
  DCHECK(bitmap_.get());

  if (!bitmap_.get()) return;

  cc::SoftwareOutputDevice::EndPaint();

  if (active_)
    OnPaint(damage_rect_);
}

void OffScreenOutputDevice::SetActive(bool active) {
  if (active == active_)
    return;
  active_ = active;

  if (active_)
    OnPaint(gfx::Rect(viewport_pixel_size_));
}

void OffScreenOutputDevice::OnPaint(const gfx::Rect& damage_rect) {
  gfx::Rect rect = damage_rect;
  if (!pending_damage_rect_.IsEmpty()) {
    rect.Union(pending_damage_rect_);
    pending_damage_rect_.SetRect(0, 0, 0, 0);
  }

  rect.Intersect(gfx::Rect(viewport_pixel_size_));
  if (rect.IsEmpty())
    return;

  SkAutoLockPixels bitmap_pixels_lock(*bitmap_);
  callback_.Run(rect, gfx::Size(bitmap_->width(), bitmap_->height()),
                bitmap_->getPixels());
}

}  // namespace atom
