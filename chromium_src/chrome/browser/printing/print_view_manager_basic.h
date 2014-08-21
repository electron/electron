// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_BASIC_H_
#define CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_BASIC_H_

#include "chrome/browser/printing/print_view_manager_base.h"
#include "content/public/browser/web_contents_user_data.h"

#if defined(OS_ANDROID)
#include "base/file_descriptor_posix.h"
#endif

namespace printing {

// Manages the print commands for a WebContents - basic version.
class PrintViewManagerBasic
    : public PrintViewManagerBase,
      public content::WebContentsUserData<PrintViewManagerBasic> {
 public:
  virtual ~PrintViewManagerBasic();

#if defined(OS_ANDROID)
  // Sets the file descriptor into which the PDF will be written.
  void set_file_descriptor(const base::FileDescriptor& file_descriptor) {
    file_descriptor_ = file_descriptor;
  }

  // Gets the file descriptor into which the PDF will be written.
  base::FileDescriptor file_descriptor() const { return file_descriptor_; }

  // content::WebContentsObserver implementation.
  // Terminates or cancels the print job if one was pending.
  virtual void RenderProcessGone(base::TerminationStatus status) OVERRIDE;

  // content::WebContentsObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
#endif

 private:
  explicit PrintViewManagerBasic(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PrintViewManagerBasic>;

#if defined(OS_ANDROID)
  virtual void OnPrintingFailed(int cookie) OVERRIDE;

  // The file descriptor into which the PDF of the page will be written.
  base::FileDescriptor file_descriptor_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PrintViewManagerBasic);
};

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINT_VIEW_MANAGER_BASIC_H_
