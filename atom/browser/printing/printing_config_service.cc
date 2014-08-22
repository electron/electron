// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/printing/printing_config_service.h"

#include "base/memory/singleton.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/printing/printing_ui_web_contents_observer.h"
#include "chrome/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;

namespace atom {

namespace {

void RenderParamsFromPrintSettings(const printing::PrintSettings& settings,
                                   PrintMsg_Print_Params* params) {
  params->page_size = settings.page_setup_device_units().physical_size();
  params->content_size.SetSize(
      settings.page_setup_device_units().content_area().width(),
      settings.page_setup_device_units().content_area().height());
  params->printable_area.SetRect(
      settings.page_setup_device_units().printable_area().x(),
      settings.page_setup_device_units().printable_area().y(),
      settings.page_setup_device_units().printable_area().width(),
      settings.page_setup_device_units().printable_area().height());
  params->margin_top = settings.page_setup_device_units().content_area().y();
  params->margin_left = settings.page_setup_device_units().content_area().x();
  params->dpi = settings.dpi();
  // Currently hardcoded at 1.25. See PrintSettings' constructor.
  params->min_shrink = settings.min_shrink();
  // Currently hardcoded at 2.0. See PrintSettings' constructor.
  params->max_shrink = settings.max_shrink();
  // Currently hardcoded at 72dpi. See PrintSettings' constructor.
  params->desired_dpi = settings.desired_dpi();
  // Always use an invalid cookie.
  params->document_cookie = 0;
  params->selection_only = settings.selection_only();
  params->supports_alpha_blend = settings.supports_alpha_blend();
  params->should_print_backgrounds = settings.should_print_backgrounds();
  params->title = settings.title();
  params->url = settings.url();
}

}  // namespace

// static
PrintingConfigService* PrintingConfigService::GetInstance() {
  return Singleton<PrintingConfigService>::get();
}

PrintingConfigService::PrintingConfigService()
    : weak_factory_(this) {
}

PrintingConfigService::~PrintingConfigService() {
}

void PrintingConfigService::GetPrintSettings(
    content::WebContents* wc,
    scoped_refptr<printing::PrinterQuery> printer_query,
    bool ask_user_for_settings,
    const PrintHostMsg_ScriptedPrint_Params& params,
    PrintSettingsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (wc) {
    scoped_ptr<PrintingUIWebContentsObserver> wc_observer(
        new PrintingUIWebContentsObserver(wc));
    printing::PrinterQuery::GetSettingsAskParam ask_param =
        ask_user_for_settings ? printing::PrinterQuery::ASK_USER :
                                printing::PrinterQuery::DEFAULTS;
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&printing::PrinterQuery::GetSettings, printer_query,
                   ask_param, base::Passed(&wc_observer),
                   params.expected_pages_count, params.has_selection,
                   params.margin_type,
                   base::Bind(&PrintingConfigService::OnGetSettings,
                              weak_factory_.GetWeakPtr(), printer_query,
                              callback)));
  } else {
  }
}

void PrintingConfigService::OnGetSettings(
    scoped_refptr<printing::PrinterQuery> printer_query,
    PrintSettingsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  PrintMsg_PrintPages_Params params;
  if (printer_query->last_status() != printing::PrintingContext::OK ||
      !printer_query->settings().dpi()) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params.params);
    params.params.document_cookie = printer_query->cookie();
    params.pages =
        printing::PageRange::GetPages(printer_query->settings().ranges());
  }
  callback.Run(params);
}

void PrintingConfigService::OnGetSettingsFailed(
    scoped_refptr<printing::PrinterQuery> printer_query,
    PrintSettingsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  printer_query->GetSettingsDone(printing::PrintSettings(),
                                 printing::PrintingContext::FAILED);
  callback.Run(PrintMsg_PrintPages_Params());
}

}  // namespace atom
