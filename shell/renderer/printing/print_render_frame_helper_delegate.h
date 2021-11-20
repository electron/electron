// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
#define ELECTRON_SHELL_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_

#include "components/printing/renderer/print_render_frame_helper.h"

namespace electron {

class PrintRenderFrameHelperDelegate
    : public printing::PrintRenderFrameHelper::Delegate {
 public:
  PrintRenderFrameHelperDelegate();
  ~PrintRenderFrameHelperDelegate() override;

  // disable copy
  PrintRenderFrameHelperDelegate(const PrintRenderFrameHelperDelegate&) =
      delete;
  PrintRenderFrameHelperDelegate& operator=(
      const PrintRenderFrameHelperDelegate&) = delete;

 private:
  // printing::PrintRenderFrameHelper::Delegate:
  blink::WebElement GetPdfElement(blink::WebLocalFrame* frame) override;
  bool IsPrintPreviewEnabled() override;
  bool OverridePrint(blink::WebLocalFrame* frame) override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
