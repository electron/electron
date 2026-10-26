// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/queue.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "chrome/browser/printing/print_view_manager_base.h"
#include "components/printing/browser/print_to_pdf/pdf_print_job.h"
#include "components/printing/common/print.mojom.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class RenderFrameHost;
}

namespace printing {
class PrinterQuery;
class PrintSettings;
}  // namespace printing

namespace electron {

using PrintToPdfCallback = print_to_pdf::PdfPrintJob::PrintToPdfCallback;

class PrintViewManagerElectron
    : public printing::PrintViewManagerBase,
      public content::WebContentsUserData<PrintViewManagerElectron> {
 public:
  ~PrintViewManagerElectron() override;

  PrintViewManagerElectron(const PrintViewManagerElectron&) = delete;
  PrintViewManagerElectron& operator=(const PrintViewManagerElectron&) = delete;

  static void BindPrintManagerHost(
      mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost>
          receiver,
      content::RenderFrameHost* rfh);

  void DidPrintToPdf(int cookie,
                     PrintToPdfCallback callback,
                     print_to_pdf::PdfPrintResult result,
                     scoped_refptr<base::RefCountedMemory> memory);
  void PrintToPdf(content::RenderFrameHost* rfh,
                  const std::string& page_ranges,
                  printing::mojom::PrintPagesParamsPtr print_page_params,
                  PrintToPdfCallback callback);

 private:
  friend class content::WebContentsUserData<PrintViewManagerElectron>;

  explicit PrintViewManagerElectron(content::WebContents* web_contents);

  // printing::mojom::PrintManagerHost:
  void DidGetPrintedPagesCount(int32_t cookie, uint32_t number_pages) override;
  void GetDefaultPrintSettings(
      GetDefaultPrintSettingsCallback callback) override;
  void ScriptedPrint(printing::mojom::ScriptedPrintParamsPtr params,
                     ScriptedPrintCallback callback) override;
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  void SetAccessibilityTree(
      int32_t cookie,
      const ui::AXTreeUpdate& accessibility_tree) override;
  void GetPrintPreviewParams(GetPrintPreviewParamsCallback callback) override;
  void UpdatePrintSettings(base::DictValue job_settings,
                           UpdatePrintSettingsCallback callback) override;
  void SetupScriptedPrintPreview(
      SetupScriptedPrintPreviewCallback callback) override;
  void ShowScriptedPrintPreview() override;
  void RequestPrintPreview(
      printing::mojom::RequestPrintPreviewParamsPtr params) override;
  void CheckForCancel(const base::UnguessableToken& preview_ui_id,
                      int32_t request_id,
                      CheckForCancelCallback callback) override;

  // Queues `settings` to be consumed by the next GetPrintPreviewParams() call.
  void AppendPrintPreviewSettings(base::DictValue settings, bool is_pdf);

  // Completes GetPrintPreviewParams() once the PrinterQuery has resolved the
  // requested settings against the target printer.
  void CompleteGetPrintPreviewParams(
      std::unique_ptr<printing::PrinterQuery> printer_query,
      base::DictValue job_settings,
      std::unique_ptr<printing::PrintSettings> print_settings,
      GetPrintPreviewParamsCallback callback);
#endif
  std::vector<int32_t> pdf_jobs_;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // Pending job settings queued by UpdatePrintSettings() and consumed by
  // GetPrintPreviewParams().
  base::queue<base::DictValue> print_preview_settings_;
#endif

  base::WeakPtrFactory<PrintViewManagerElectron> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
