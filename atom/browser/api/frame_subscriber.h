// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
#define ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_

#include "content/public/browser/web_contents.h"

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/gfx/image/image.h"
#include "v8/include/v8.h"

namespace atom {

namespace api {

class WebContents;

class FrameSubscriber : public content::WebContentsObserver {
 public:
  using FrameCaptureCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  FrameSubscriber(v8::Isolate* isolate,
                  content::WebContents* web_contents,
                  const FrameCaptureCallback& callback,
                  bool only_dirty);
  ~FrameSubscriber();

 private:
  gfx::Rect GetDamageRect();
  void DidReceiveCompositorFrame() override;
  void Done(const gfx::Rect& damage, const SkBitmap& frame);

  v8::Isolate* isolate_;
  FrameCaptureCallback callback_;
  bool only_dirty_;

  base::WeakPtrFactory<FrameSubscriber> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_FRAME_SUBSCRIBER_H_
