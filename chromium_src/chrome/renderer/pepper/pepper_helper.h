// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PEPPER_PEPPER_HELPER_H_
#define CHROME_RENDERER_PEPPER_PEPPER_HELPER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_frame_observer.h"

// This class listens for Pepper creation events from the RenderFrame and
// attaches the parts required for Chrome-specific plugin support.
class PepperHelper : public content::RenderFrameObserver {
 public:
  explicit PepperHelper(content::RenderFrame* render_frame);
  ~PepperHelper() override;

  // RenderFrameObserver.
  void DidCreatePepperPlugin(content::RendererPpapiHost* host) override;
  void OnDestruct() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperHelper);
};

#endif  // CHROME_RENDERER_PEPPER_PEPPER_HELPER_H_
