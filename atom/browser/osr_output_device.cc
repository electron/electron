// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr_output_device.h"

#include <iostream>

namespace atom {

OffScreenOutputDevice::OffScreenOutputDevice() {
  std::cout << "OffScreenOutputDevice" << std::endl;
}

OffScreenOutputDevice::~OffScreenOutputDevice() {
  std::cout << "~OffScreenOutputDevice" << std::endl;
}

void OffScreenOutputDevice::Resize(
  const gfx::Size& pixel_size, float scale_factor) {
  std::cout << "Resize" << std::endl;
  std::cout << pixel_size.width() << "x" << pixel_size.height() << std::endl;

  scale_factor_ = scale_factor;

  if (viewport_pixel_size_ == pixel_size) return;
  viewport_pixel_size_ = pixel_size;

  canvas_.reset(NULL);
  bitmap_.reset(new SkBitmap);
  bitmap_->allocN32Pixels(viewport_pixel_size_.width(),
                          viewport_pixel_size_.height(),
                          false);
  if (bitmap_->drawsNothing()) {
    std::cout << "drawsNothing" << std::endl;
    NOTREACHED();
    bitmap_.reset(NULL);
    return;
  }
  bitmap_->eraseARGB(0, 0, 0, 0);

  canvas_.reset(new SkCanvas(*bitmap_.get()));
}

SkCanvas* OffScreenOutputDevice::BeginPaint(const gfx::Rect& damage_rect) {
  std::cout << "BeginPaint" << std::endl;
  DCHECK(canvas_.get());
  DCHECK(bitmap_.get());

  damage_rect_ = damage_rect;

  return canvas_.get();
}

void OffScreenOutputDevice::EndPaint() {
  std::cout << "EndPaint" << std::endl;

  DCHECK(canvas_.get());
  DCHECK(bitmap_.get());

  if (!bitmap_.get()) return;

  cc::SoftwareOutputDevice::EndPaint();

  SkAutoLockPixels bitmap_pixels_lock(*bitmap_.get());
  //saveSkBitmapToBMPFile(*(bitmap_.get()), "test.bmp");

  uint8_t* pixels = reinterpret_cast<uint8_t*>(bitmap_->getPixels());
  for (int i = 0; i<16; i++) {
    int x = static_cast<int>(pixels[i]);
    std::cout << std::hex << x << std::dec << std::endl;
  }
}

}  // namespace atom
