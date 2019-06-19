// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
#define ATOM_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_

#include "base/macros.h"
#include "components/printing/renderer/print_render_frame_helper.h"

namespace atom {

class PrintRenderFrameHelperDelegate
    : public printing::PrintRenderFrameHelper::Delegate {
 public:
  PrintRenderFrameHelperDelegate();
  ~PrintRenderFrameHelperDelegate() override;

 private:
  // printing::PrintRenderFrameHelper::Delegate:
  bool CancelPrerender(content::RenderFrame* render_frame) override;
  blink::WebElement GetPdfElement(blink::WebLocalFrame* frame) override;
  bool IsPrintPreviewEnabled() override;
  bool OverridePrint(blink::WebLocalFrame* frame) override;

  DISALLOW_COPY_AND_ASSIGN(PrintRenderFrameHelperDelegate);
};

}  // namespace atom

#endif  // ATOM_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
