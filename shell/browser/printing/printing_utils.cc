// Copyright (c) 2024 Microsoft, GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/printing_utils.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "chrome/browser/browser_process.h"
#include "components/pdf/browser/pdf_frame_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "electron/buildflags/buildflags.h"
#include "printing/backend/print_backend.h"
#include "printing/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_OOP_PRINTING)
#include "chrome/browser/printing/oop_features.h"
#include "chrome/browser/printing/print_backend_service_manager.h"
#include "chrome/services/printing/public/mojom/print_backend_service.mojom.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "pdf/pdf_features.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "shell/common/thread_restrictions.h"
#endif

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PDF_VIEWER)
// Duplicated from chrome/browser/printing/print_view_manager_common.cc
content::RenderFrameHost* GetFullPagePlugin(content::WebContents* contents) {
  content::RenderFrameHost* full_page_plugin = nullptr;
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  contents->ForEachRenderFrameHostWithAction(
      [&full_page_plugin](content::RenderFrameHost* rfh) {
        auto* guest_view =
            extensions::MimeHandlerViewGuest::FromRenderFrameHost(rfh);
        if (guest_view && guest_view->is_full_page_plugin()) {
          DCHECK_EQ(guest_view->GetGuestMainFrame(), rfh);
          full_page_plugin = rfh;
          return content::RenderFrameHost::FrameIterationAction::kStop;
        }
        return content::RenderFrameHost::FrameIterationAction::kContinue;
      });
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  return full_page_plugin;
}
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace

// Pick the right RenderFrameHost based on the WebContents.
// Modified from chrome/browser/printing/print_view_manager_common.cc
content::RenderFrameHost* GetRenderFrameHostToUse(
    content::WebContents* contents) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  // Pick the plugin frame host if `contents` is a PDF viewer guest. If using
  // OOPIF PDF viewer, pick the PDF extension frame host.
  content::RenderFrameHost* full_page_pdf_embedder_host =
      chrome_pdf::features::IsOopifPdfEnabled()
          ? pdf_frame_util::FindFullPagePdfExtensionHost(contents)
          : GetFullPagePlugin(contents);
  content::RenderFrameHost* pdf_rfh = pdf_frame_util::FindPdfChildFrame(
      full_page_pdf_embedder_host ? full_page_pdf_embedder_host
                                  : contents->GetPrimaryMainFrame());
  if (pdf_rfh) {
    return pdf_rfh;
  }
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
  auto* focused_frame = contents->GetFocusedFrame();
  return (focused_frame && focused_frame->HasSelection())
             ? focused_frame
             : contents->GetPrimaryMainFrame();
}

namespace {

// As in Chrome's LocalPrinterHandlerDefault: CUPS is thread safe, Windows
// drivers must be used from the UI thread.
scoped_refptr<base::TaskRunner> PrintBackendTaskRunner() {
#if BUILDFLAG(IS_WIN)
  return content::GetUIThreadTaskRunner({base::TaskPriority::USER_VISIBLE});
#else
  static base::NoDestructor<scoped_refptr<base::TaskRunner>> runner(
      base::ThreadPool::CreateTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE}));
  return *runner;
#endif
}

scoped_refptr<printing::PrintBackend> CreateBackend() {
  return printing::PrintBackend::CreateInstance(
      g_browser_process->GetApplicationLocale());
}

printing::PrinterList EnumeratePrintersBlocking() {
#if BUILDFLAG(IS_WIN)
  ScopedAllowBlockingForElectron allow_blocking;
#endif
  printing::PrinterList printers;
  CreateBackend()->EnumeratePrinters(printers);
  return printers;
}

std::string GetDefaultPrinterNameBlocking() {
#if BUILDFLAG(IS_WIN)
  ScopedAllowBlockingForElectron allow_blocking;
#endif
  std::string name;
  CreateBackend()->GetDefaultPrinterName(name);
  return name;
}

struct PrinterLookup {
  bool exists = false;
  std::optional<printing::PrinterSemanticCapsAndDefaults> caps;
};

PrinterLookup LookUpPrinterBlocking(std::string name, bool want_caps) {
#if BUILDFLAG(IS_WIN)
  ScopedAllowBlockingForElectron allow_blocking;
#endif
  scoped_refptr<printing::PrintBackend> backend = CreateBackend();
  PrinterLookup result;
  result.exists = backend->IsValidPrinter(name);
  printing::PrinterSemanticCapsAndDefaults caps;
  if (result.exists && want_caps &&
      backend->GetPrinterSemanticCapsAndDefaults(name, &caps) ==
          printing::mojom::ResultCode::kSuccess) {
    result.caps = std::move(caps);
  }
  return result;
}

#if BUILDFLAG(ENABLE_OOP_PRINTING)
// Holds a query-client registration for the lifetime of one service call.
class ScopedQueryClient {
 public:
  ScopedQueryClient()
      : id_(printing::PrintBackendServiceManager::GetInstance()
                .RegisterQueryClient()) {}
  ScopedQueryClient(ScopedQueryClient&& other)
      : id_(std::exchange(other.id_, std::nullopt)) {}
  ~ScopedQueryClient() {
    if (id_) {
      printing::PrintBackendServiceManager::GetInstance().UnregisterClient(
          *id_);
    }
  }

 private:
  std::optional<printing::PrintBackendServiceManager::ClientId> id_;
};
#endif

void GetDefaultPrinterName(base::OnceCallback<void(std::string)> callback) {
#if BUILDFLAG(ENABLE_OOP_PRINTING)
  if (printing::IsOopPrintingEnabled()) {
    ScopedQueryClient client;
    printing::PrintBackendServiceManager::GetInstance().GetDefaultPrinterName(
        base::BindOnce(
            [](ScopedQueryClient,
               base::OnceCallback<void(std::string)> callback,
               base::expected<std::string, printing::mojom::ResultCode> name) {
              std::move(callback).Run(name.value_or(""));
            },
            std::move(client), std::move(callback)));
    return;
  }
#endif
  PrintBackendTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GetDefaultPrinterNameBlocking),
      std::move(callback));
}

using LookupCallback = base::OnceCallback<void(PrinterLookup)>;

void LookUpPrinter(const std::string& name,
                   bool want_caps,
                   LookupCallback callback) {
#if BUILDFLAG(ENABLE_OOP_PRINTING)
  if (printing::IsOopPrintingEnabled()) {
    // FetchCapabilities fails for queues without a PPD, so learn whether the
    // printer exists from the list and treat capabilities as optional.
    GetPrinterList(base::BindOnce(
        [](std::string name, bool want_caps, LookupCallback callback,
           printing::PrinterList printers) {
          if (!std::ranges::contains(
                  printers, name, &printing::PrinterBasicInfo::printer_name)) {
            std::move(callback).Run(PrinterLookup());
            return;
          }
          if (!want_caps) {
            PrinterLookup lookup;
            lookup.exists = true;
            std::move(callback).Run(std::move(lookup));
            return;
          }
          ScopedQueryClient client;
          printing::PrintBackendServiceManager::GetInstance().FetchCapabilities(
              name,
              base::BindOnce(
                  [](ScopedQueryClient, LookupCallback callback,
                     base::expected<printing::mojom::PrinterCapsAndInfoPtr,
                                    printing::mojom::ResultCode> result) {
                    PrinterLookup lookup;
                    lookup.exists = true;
                    if (result.has_value())
                      lookup.caps = std::move((*result)->printer_caps);
                    std::move(callback).Run(std::move(lookup));
                  },
                  std::move(client), std::move(callback)));
        },
        name, want_caps, std::move(callback)));
    return;
  }
#endif
  PrintBackendTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LookUpPrinterBlocking, name, want_caps),
      std::move(callback));
}

void OnPrinterLookup(std::string name,
                     ResolvePrinterCallback callback,
                     PrinterLookup lookup) {
  if (!lookup.exists) {
    std::move(callback).Run(base::unexpected("Invalid deviceName provided"));
    return;
  }
  ResolvedPrinter printer;
  printer.name = std::move(name);
  if (lookup.caps) {
    printer.dpi = !lookup.caps->default_dpi.IsEmpty() ? lookup.caps->default_dpi
                  : !lookup.caps->dpis.empty() ? lookup.caps->dpis.front()
                                               : gfx::Size();
    printer.default_paper_um = lookup.caps->default_paper.size_um();
  }
  std::move(callback).Run(std::move(printer));
}

void OnPrinterList(bool want_caps,
                   ResolvePrinterCallback callback,
                   printing::PrinterList printers) {
  if (printers.empty()) {
    std::move(callback).Run(
        base::unexpected("No printers available on the network"));
    return;
  }
  std::string name = printers.front().printer_name;
  LookUpPrinter(name, want_caps,
                base::BindOnce(&OnPrinterLookup, name, std::move(callback)));
}

void OnDefaultPrinterName(bool want_caps,
                          ResolvePrinterCallback callback,
                          std::string name) {
  if (name.empty()) {
    GetPrinterList(
        base::BindOnce(&OnPrinterList, want_caps, std::move(callback)));
    return;
  }
  LookUpPrinter(name, want_caps,
                base::BindOnce(&OnPrinterLookup, name, std::move(callback)));
}

}  // namespace

ResolvedPrinter::ResolvedPrinter() = default;
ResolvedPrinter::ResolvedPrinter(ResolvedPrinter&&) = default;
ResolvedPrinter& ResolvedPrinter::operator=(ResolvedPrinter&&) = default;
ResolvedPrinter::~ResolvedPrinter() = default;

void GetPrinterList(base::OnceCallback<void(printing::PrinterList)> callback) {
#if BUILDFLAG(ENABLE_OOP_PRINTING)
  if (printing::IsOopPrintingEnabled()) {
    ScopedQueryClient client;
    printing::PrintBackendServiceManager::GetInstance().EnumeratePrinters(
        base::BindOnce(
            [](ScopedQueryClient,
               base::OnceCallback<void(printing::PrinterList)> callback,
               base::expected<printing::PrinterList,
                              printing::mojom::ResultCode> printers) {
              std::move(callback).Run(
                  std::move(printers).value_or(printing::PrinterList()));
            },
            std::move(client), std::move(callback)));
    return;
  }
#endif
  PrintBackendTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&EnumeratePrintersBlocking),
      std::move(callback));
}

void ResolvePrinter(const std::string& device_name,
                    bool want_caps,
                    ResolvePrinterCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!device_name.empty()) {
    LookUpPrinter(
        device_name, want_caps,
        base::BindOnce(&OnPrinterLookup, device_name, std::move(callback)));
    return;
  }
  GetDefaultPrinterName(
      base::BindOnce(&OnDefaultPrinterName, want_caps, std::move(callback)));
}

}  // namespace electron
