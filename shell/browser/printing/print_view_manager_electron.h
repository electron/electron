// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
#define ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "chrome/browser/printing/print_job.h"
#include "chrome/browser/printing/print_view_manager_base.h"
#include "components/printing/common/print.mojom.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents_user_data.h"
#include "printing/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_OOP_PRINTING)
#include "chrome/browser/printing/print_backend_service_manager.h"
#endif

namespace content {
class RenderFrameHost;
}

namespace printing {
class PrinterQuery;
}

namespace electron {

// Handles window.print() through PrintViewManagerBase unchanged, and runs
// webContents.print() as its own job: settings are resolved on a PrinterQuery,
// the frame renders them via PrintRenderFrame::PrintWithParams(), and the
// result is spooled through a PrintJob this class creates and observes.
class PrintViewManagerElectron
    : public printing::PrintViewManagerBase,
      public content::WebContentsUserData<PrintViewManagerElectron> {
 public:
  using PrintCallback =
      base::OnceCallback<void(bool success, const std::string& reason)>;

  ~PrintViewManagerElectron() override;

  PrintViewManagerElectron(const PrintViewManagerElectron&) = delete;
  PrintViewManagerElectron& operator=(const PrintViewManagerElectron&) = delete;

  static void BindPrintManagerHost(
      mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost>
          receiver,
      content::RenderFrameHost* rfh);

  // `settings` is a complete job settings dictionary, or nullopt for the
  // system defaults; without `silent` the system dialog confirms them first.
  void Print(content::RenderFrameHost* rfh,
             std::optional<base::DictValue> settings,
             bool silent,
             PrintCallback callback);

 private:
  friend class content::WebContentsUserData<PrintViewManagerElectron>;

  class JobObserver : public printing::PrintJob::Observer {
   public:
    explicit JobObserver(PrintViewManagerElectron* owner) : owner_(owner) {}
    void OnJobDone() override;
    void OnCanceling() override;
    void OnFailed() override;

   private:
    const raw_ptr<PrintViewManagerElectron> owner_;
  };

  struct Job {
    Job();
    Job(Job&&);
    ~Job();

    int id = 0;
    content::GlobalRenderFrameHostId rfh_id;
    PrintCallback callback;
    bool silent = false;
    // Null while a settings request or the dialog is pending; the query is
    // owned by that callback so it outlives `this`, as upstream does.
    std::unique_ptr<printing::PrinterQuery> query;
    scoped_refptr<printing::PrintJob> print_job;
#if BUILDFLAG(ENABLE_OOP_PRINTING)
    std::optional<printing::PrintBackendServiceManager::ClientId>
        dialog_client_id;
#endif
  };

  explicit PrintViewManagerElectron(content::WebContents* web_contents);

  bool IsCurrentJob(int id) const;
  bool RegisterDialogClient(bool assign_to_query);
  void OnSettingsResolved(int id,
                          std::unique_ptr<printing::PrinterQuery> query);
  void ShowDialog();
  void OnDialogDone(int id, std::unique_ptr<printing::PrinterQuery> query);
  void RenderDocument();
  void OnDocumentRendered(
      int id,
      printing::mojom::PrintRenderFrame::PrintWithParamsResult result);
  void OnDocumentComposited(int id,
                            uint32_t page_count,
                            printing::mojom::DidPrintDocumentParamsPtr params,
                            printing::mojom::PrintCompositor::Status status,
                            base::ReadOnlySharedMemoryRegion pdf);
  void SpoolDocument(uint32_t page_count,
                     printing::mojom::DidPrintDocumentParamsPtr params,
                     base::ReadOnlySharedMemoryRegion pdf);
  void Finish(bool success, std::string_view reason);

  // content::WebContentsObserver:
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // printing::mojom::PrintManagerHost: the preview UI is not built.
  void SetAccessibilityTree(
      int32_t cookie,
      const ui::AXTreeUpdate& accessibility_tree) override;
  void GetPrintPreviewParams(GetPrintPreviewParamsCallback callback) override;
  void SetupScriptedPrintPreview(
      SetupScriptedPrintPreviewCallback callback) override;
  void ShowScriptedPrintPreview() override;
  void RequestPrintPreview(
      printing::mojom::RequestPrintPreviewParamsPtr params) override;
  void CheckForCancel(const base::UnguessableToken& preview_ui_id,
                      int32_t request_id,
                      CheckForCancelCallback callback) override;
#endif

  std::optional<Job> job_;
  int next_job_id_ = 0;
  JobObserver job_observer_{this};

  base::WeakPtrFactory<PrintViewManagerElectron> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
