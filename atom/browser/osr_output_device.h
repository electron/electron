// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_OUTPUT_DEVICE_H_
#define ATOM_BROWSER_OSR_OUTPUT_DEVICE_H_

#include "base/callback.h"
#include "cc/output/software_output_device.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "base/callback.h"

namespace atom {
  
typedef base::Callback<void(const gfx::Rect&,int,int,void*)> OnPaintCallback;

class OffScreenOutputDevice : public cc::SoftwareOutputDevice {
public:
  typedef base::Callback<void(const gfx::Rect&,int,int,void*)>
      OnPaintCallback;

  OffScreenOutputDevice(const OnPaintCallback& callback);
  ~OffScreenOutputDevice();
  
  void SetPaintCallback(const OnPaintCallback*);

  // void saveSkBitmapToBMPFile(const SkBitmap& skBitmap, const char* path);
  void Resize(const gfx::Size& pixel_size, float scale_factor) override;

  SkCanvas* BeginPaint(const gfx::Rect& damage_rect) override;

  void EndPaint() override;
  
  void OnPaint(const gfx::Rect& damage_rect);

  void SetActive(bool active);

  void OnPaint(const gfx::Rect& damage_rect);

private:
  const OnPaintCallback callback_;

  bool active_;

  std::unique_ptr<SkCanvas> canvas_;
  std::unique_ptr<SkBitmap> bitmap_;
  gfx::Rect pending_damage_rect_;
  
  std::unique_ptr<const atom::OnPaintCallback> callback_;

  DISALLOW_COPY_AND_ASSIGN(OffScreenOutputDevice);
};

}  // namespace atom

#endif  // ATOM_BROWSER_OSR_OUTPUT_DEVICE_H_
