// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/renderer/pepper_helper.h"

#include <memory>

#include "chrome/renderer/pepper/chrome_renderer_pepper_host_factory.h"
#include "chrome/renderer/pepper/pepper_shared_memory_message_filter.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "electron/shell/renderer/electron_renderer_pepper_host_factory.h"
#include "ppapi/host/ppapi_host.h"

PepperHelper::PepperHelper(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

PepperHelper::~PepperHelper() = default;

void PepperHelper::DidCreatePepperPlugin(content::RendererPpapiHost* host) {
  // TODO(brettw) figure out how to hook up the host factory. It needs some
  // kind of filter-like system to allow dynamic additions.
  host->GetPpapiHost()->AddHostFactoryFilter(
      std::make_unique<ChromeRendererPepperHostFactory>(host));
  host->GetPpapiHost()->AddHostFactoryFilter(
      std::make_unique<ElectronRendererPepperHostFactory>(host));
  host->GetPpapiHost()->AddInstanceMessageFilter(
      std::make_unique<PepperSharedMemoryMessageFilter>(host));
}

void PepperHelper::OnDestruct() {
  delete this;
}
