// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_view_manager_basic.h"

#include "build/build_config.h"

#if defined(OS_ANDROID)
#include "base/bind.h"
#include "printing/printing_context_android.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintViewManagerBasic);

namespace printing {

PrintViewManagerBasic::PrintViewManagerBasic(content::WebContents* web_contents)
    : PrintViewManagerBase(web_contents) {
#if defined(OS_ANDROID)
  pdf_writing_done_callback_ =
      base::Bind(&PrintingContextAndroid::PdfWritingDone);
#endif
}

PrintViewManagerBasic::~PrintViewManagerBasic() {
}

}  // namespace printing
