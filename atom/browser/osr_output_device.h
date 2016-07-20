// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_OSR_OUTPUT_DEVICE_H_
#define ATOM_BROWSER_OSR_OUTPUT_DEVICE_H_

// #include "content/browser/renderer_host/render_widget_host_view_base.h"
// #include "content/browser/renderer_host/delegated_frame_host.h"
// #include "content/browser/renderer_host/resize_lock.h"
// #include "third_party/WebKit/public/platform/WebVector.h"
// #include "cc/scheduler/begin_frame_source.h"
// #include "content/browser/renderer_host/render_widget_host_impl.h"
// #include "cc/output/compositor_frame.h"
// #include "ui/gfx/geometry/point.h"
// #include "base/threading/thread.h"
// #include "ui/compositor/compositor.h"
// #include "ui/compositor/layer_delegate.h"
// #include "ui/compositor/layer_owner.h"
// #include "ui/base/ime/text_input_client.h"

#include "cc/output/software_output_device.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

#include "content/browser/web_contents/web_contents_view.h"

namespace atom {

class OffScreenOutputDevice : public cc::SoftwareOutputDevice {
public:
  OffScreenOutputDevice();
  ~OffScreenOutputDevice();


  // void saveSkBitmapToBMPFile(const SkBitmap& skBitmap, const char* path);
  void Resize(const gfx::Size& pixel_size, float scale_factor) override;

  SkCanvas* BeginPaint(const gfx::Rect& damage_rect) override;

  void EndPaint() override;

private:
  std::unique_ptr<SkCanvas> canvas_;
  std::unique_ptr<SkBitmap> bitmap_;
  gfx::Rect pending_damage_rect_;

  DISALLOW_COPY_AND_ASSIGN(OffScreenOutputDevice);
};


}  // namespace atom

#endif  // ATOM_BROWSER_OSR_OUTPUT_DEVICE_H_
