// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_
#define CHROME_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_

#include <string>

#include "base/compiler_specific.h"
#include "content/public/browser/browser_message_filter.h"

struct PrintHostMsg_ScriptedPrint_Params;

namespace base {
class DictionaryValue;
class FilePath;
}

namespace content {
class WebContents;
}

namespace printing {

class PrintQueriesQueue;
class PrinterQuery;

// This class filters out incoming printing related IPC messages for the
// renderer process on the IPC thread.
class PrintingMessageFilter : public content::BrowserMessageFilter {
 public:
  PrintingMessageFilter(int render_process_id);

  // content::BrowserMessageFilter methods.
  void OverrideThreadForMessage(
      const IPC::Message& message,
      content::BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  friend class base::DeleteHelper<PrintingMessageFilter>;
  friend class content::BrowserThread;

  virtual ~PrintingMessageFilter();

  void OnDestruct() const override;

#if defined(OS_ANDROID)
  // Used to ask the browser allocate a temporary file for the renderer
  // to fill in resulting PDF in renderer.
  void OnAllocateTempFileForPrinting(int render_frame_id,
                                     base::FileDescriptor* temp_file_fd,
                                     int* sequence_number);
  void OnTempFileForPrintingWritten(int render_frame_id, int sequence_number);

  // Updates the file descriptor for the PrintViewManagerBasic of a given
  // render_frame_id.
  void UpdateFileDescriptor(int render_frame_id, int fd);
#endif

  // Get the default print setting.
  void OnGetDefaultPrintSettings(IPC::Message* reply_msg);

  // Set deviceName
  void OnInitSettingWithDeviceName(const base::string16& device_name,
                                   IPC::Message* reply_msg);

  void OnGetDefaultPrintSettingsReply(scoped_refptr<PrinterQuery> printer_query,
                                      IPC::Message* reply_msg);

  // The renderer host have to show to the user the print dialog and returns
  // the selected print settings. The task is handled by the print worker
  // thread and the UI thread. The reply occurs on the IO thread.
  void OnScriptedPrint(const PrintHostMsg_ScriptedPrint_Params& params,
                       IPC::Message* reply_msg);
  void OnScriptedPrintReply(scoped_refptr<PrinterQuery> printer_query,
                            IPC::Message* reply_msg);

  // Modify the current print settings based on |job_settings|. The task is
  // handled by the print worker thread and the UI thread. The reply occurs on
  // the IO thread.
  void OnUpdatePrintSettings(int document_cookie,
                             const base::DictionaryValue& job_settings,
                             IPC::Message* reply_msg);
  void OnUpdatePrintSettingsReply(scoped_refptr<PrinterQuery> printer_query,
                                  IPC::Message* reply_msg);

  const int render_process_id_;

  scoped_refptr<PrintQueriesQueue> queue_;

  DISALLOW_COPY_AND_ASSIGN(PrintingMessageFilter);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINTING_MESSAGE_FILTER_H_
