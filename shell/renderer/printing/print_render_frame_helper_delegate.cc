// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/printing/print_render_frame_helper_delegate.h"

#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/common/extensions/extension_constants.h"
#include "extensions/common/constants.h"
#include "extensions/renderer/guest_view/mime_handler_view/post_message_support.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace electron {

PrintRenderFrameHelperDelegate::PrintRenderFrameHelperDelegate() = default;

PrintRenderFrameHelperDelegate::~PrintRenderFrameHelperDelegate() = default;

// Return the PDF object element if |frame| is the out of process PDF extension.
blink::WebElement PrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  GURL url = frame->GetDocument().Url();
  bool inside_print_preview = url.GetOrigin() == chrome::kChromeUIPrintURL;
  bool inside_pdf_extension =
      url.SchemeIs(extensions::kExtensionScheme) &&
      url.host_piece() == extension_misc::kPdfExtensionId;
  if (inside_print_preview || inside_pdf_extension) {
    // <object> with id="plugin" is created in
    // chrome/browser/resources/pdf/pdf_viewer_base.js.
    auto viewer_element = frame->GetDocument().GetElementById("viewer");
    if (!viewer_element.IsNull() && !viewer_element.ShadowRoot().IsNull()) {
      auto plugin_element =
          viewer_element.ShadowRoot().QuerySelector("#plugin");
      if (!plugin_element.IsNull()) {
        return plugin_element;
      }
    }
    NOTREACHED();
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
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
