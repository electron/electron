// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/printing/print_view_manager_base.h"
#include "components/printing/browser/print_to_pdf/pdf_print_job.h"
#include "components/printing/common/print.mojom.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class RenderFrameHost;
}

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
  void SetupScriptedPrintPreview(
      SetupScriptedPrintPreviewCallback callback) override;
  void ShowScriptedPrintPreview(bool source_is_modifiable) override;
  void RequestPrintPreview(
      printing::mojom::RequestPrintPreviewParamsPtr params) override;
  void CheckForCancel(int32_t preview_ui_id,
                      int32_t request_id,
                      CheckForCancelCallback callback) override;
#endif
  std::vector<int32_t> pdf_jobs_;

  base::WeakPtrFactory<PrintViewManagerElectron> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
