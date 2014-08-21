// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_view_manager_basic.h"

#if defined(OS_ANDROID)
#include "base/file_descriptor_posix.h"
#include "chrome/common/print_messages.h"
#include "printing/printing_context_android.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(printing::PrintViewManagerBasic);

namespace printing {

PrintViewManagerBasic::PrintViewManagerBasic(content::WebContents* web_contents)
    : PrintViewManagerBase(web_contents) {
}

PrintViewManagerBasic::~PrintViewManagerBasic() {
}

#if defined(OS_ANDROID)
void PrintViewManagerBasic::RenderProcessGone(base::TerminationStatus status) {
  PrintingContextAndroid::PdfWritingDone(file_descriptor_.fd, false);
  file_descriptor_ = base::FileDescriptor(-1, false);
  PrintViewManagerBase::RenderProcessGone(status);
}

void PrintViewManagerBasic::OnPrintingFailed(int cookie) {
  PrintingContextAndroid::PdfWritingDone(file_descriptor_.fd, false);
  file_descriptor_ = base::FileDescriptor(-1, false);
  PrintViewManagerBase::OnPrintingFailed(cookie);
}

bool PrintViewManagerBasic::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintViewManagerBasic, message)
    IPC_MESSAGE_HANDLER(PrintHostMsg_PrintingFailed, OnPrintingFailed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled ? true : PrintViewManagerBase::OnMessageReceived(message);
}
#endif

}  // namespace printing
