// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_
#define ATOM_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_

#include "base/compiler_specific.h"
#include "content/public/browser/browser_message_filter.h"

#if defined(OS_WIN)
#include "base/memory/shared_memory.h"
#endif

struct PrintMsg_PrintPages_Params;
struct PrintHostMsg_ScriptedPrint_Params;

namespace content {
class WebContents;
}

namespace printing {
class PrinterQuery;
class PrintQueriesQueue;
}

namespace atom {

// This class filters out incoming printing related IPC messages for the
// renderer process on the IPC thread.
class PrintingMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit PrintingMessageFilter(int render_process_id);

  // content::BrowserMessageFilter methods.
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 bool* message_was_ok) OVERRIDE;

 private:
  virtual ~PrintingMessageFilter();

#if defined(OS_WIN)
  // Used to pass resulting EMF from renderer to browser in printing.
  void OnDuplicateSection(base::SharedMemoryHandle renderer_handle,
                          base::SharedMemoryHandle* browser_handle);
#endif

  // Given a render_view_id get the corresponding WebContents.
  // Must be called on the UI thread.
  content::WebContents* GetWebContentsForRenderView(int render_view_id);

  // Retrieve print settings.  Uses |render_view_id| to get a parent
  // for any UI created if needed.
  void GetPrintSettingsForRenderView(
      int render_view_id,
      bool ask_user_for_settings,
      PrintHostMsg_ScriptedPrint_Params params,
      const base::Callback<void(const PrintMsg_PrintPages_Params&)>callback,
      scoped_refptr<printing::PrinterQuery> printer_query);

  // Get the default print setting.
  void OnGetDefaultPrintSettings(IPC::Message* reply_msg);
  void OnGetDefaultPrintSettingsReply(
      scoped_refptr<printing::PrinterQuery> printer_query,
      IPC::Message* reply_msg,
      const PrintMsg_PrintPages_Params& params);

  // The renderer host have to show to the user the print dialog and returns
  // the selected print settings. The task is handled by the print worker
  // thread and the UI thread. The reply occurs on the IO thread.
  void OnScriptedPrint(const PrintHostMsg_ScriptedPrint_Params& params,
                       IPC::Message* reply_msg);
  void OnScriptedPrintReply(
      scoped_refptr<printing::PrinterQuery> printer_query,
      IPC::Message* reply_msg,
      const PrintMsg_PrintPages_Params& params);

  const int render_process_id_;

  scoped_refptr<printing::PrintQueriesQueue> queue_;

  DISALLOW_COPY_AND_ASSIGN(PrintingMessageFilter);
};

}  // namespace atom

#endif  // ATOM_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_
