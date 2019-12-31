// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/printing/print_render_frame_helper_delegate.h"

#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

PrintRenderFrameHelperDelegate::PrintRenderFrameHelperDelegate() = default;

PrintRenderFrameHelperDelegate::~PrintRenderFrameHelperDelegate() = default;

// Return the PDF object element if |frame| is the out of process PDF extension.
blink::WebElement PrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool PrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool PrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
  return false;
}

}  // namespace electron
