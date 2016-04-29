// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/printing_message_filter.h"

#include <string>

#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/printing/printer_query.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/print_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/child_process_host.h"

#if defined(ENABLE_FULL_PRINTING)
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#endif

#if defined(OS_CHROMEOS)
#include <fcntl.h>

#include <map>

#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "chrome/browser/printing/print_dialog_cloud.h"
#endif

#if defined(OS_ANDROID)
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "printing/printing_context_android.h"
#endif

using content::BrowserThread;

namespace printing {

namespace {

#if defined(OS_CHROMEOS)
typedef std::map<int, base::FilePath> SequenceToPathMap;

struct PrintingSequencePathMap {
  SequenceToPathMap map;
  int sequence;
};

// No locking, only access on the FILE thread.
static base::LazyInstance<PrintingSequencePathMap>
    g_printing_file_descriptor_map = LAZY_INSTANCE_INITIALIZER;
#endif

void RenderParamsFromPrintSettings(const PrintSettings& settings,
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

PrintingMessageFilter::PrintingMessageFilter(int render_process_id)
    : BrowserMessageFilter(PrintMsgStart),
      render_process_id_(render_process_id),
      queue_(g_browser_process->print_job_manager()->queue()) {
  DCHECK(queue_.get());
}

PrintingMessageFilter::~PrintingMessageFilter() {
}

void PrintingMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
#if defined(OS_CHROMEOS)
  if (message.type() == PrintHostMsg_AllocateTempFileForPrinting::ID ||
      message.type() == PrintHostMsg_TempFileForPrintingWritten::ID) {
    *thread = BrowserThread::FILE;
  }
#elif defined(OS_ANDROID)
  if (message.type() == PrintHostMsg_AllocateTempFileForPrinting::ID ||
      message.type() == PrintHostMsg_TempFileForPrintingWritten::ID) {
    *thread = BrowserThread::UI;
  }
#endif
}

bool PrintingMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PrintingMessageFilter, message)
#if defined(OS_WIN)
    IPC_MESSAGE_HANDLER(PrintHostMsg_DuplicateSection, OnDuplicateSection)
#endif
#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
    IPC_MESSAGE_HANDLER(PrintHostMsg_AllocateTempFileForPrinting,
                        OnAllocateTempFileForPrinting)
    IPC_MESSAGE_HANDLER(PrintHostMsg_TempFileForPrintingWritten,
                        OnTempFileForPrintingWritten)
#endif
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_GetDefaultPrintSettings,
                                    OnGetDefaultPrintSettings)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_ScriptedPrint, OnScriptedPrint)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(PrintHostMsg_UpdatePrintSettings,
                                    OnUpdatePrintSettings)
#if defined(ENABLE_FULL_PRINTING)
    IPC_MESSAGE_HANDLER(PrintHostMsg_CheckForCancel, OnCheckForCancel)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

#if defined(OS_WIN)
void PrintingMessageFilter::OnDuplicateSection(
    base::SharedMemoryHandle renderer_handle,
    base::SharedMemoryHandle* browser_handle) {
  // Duplicate the handle in this process right now so the memory is kept alive
  // (even if it is not mapped)
  base::SharedMemory shared_buf(renderer_handle, true, PeerHandle());
  shared_buf.GiveToProcess(base::GetCurrentProcessHandle(), browser_handle);
}
#endif

#if defined(OS_CHROMEOS) || defined(OS_ANDROID)
void PrintingMessageFilter::OnAllocateTempFileForPrinting(
    int render_view_id,
    base::FileDescriptor* temp_file_fd,
    int* sequence_number) {
#if defined(OS_CHROMEOS)
  // TODO(thestig): Use |render_view_id| for Chrome OS.
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  temp_file_fd->fd = *sequence_number = -1;
  temp_file_fd->auto_close = false;

  SequenceToPathMap* map = &g_printing_file_descriptor_map.Get().map;
  *sequence_number = g_printing_file_descriptor_map.Get().sequence++;

  base::FilePath path;
  if (base::CreateTemporaryFile(&path)) {
    int fd = open(path.value().c_str(), O_WRONLY);
    if (fd >= 0) {
      SequenceToPathMap::iterator it = map->find(*sequence_number);
      if (it != map->end()) {
        NOTREACHED() << "Sequence number already in use. seq=" <<
            *sequence_number;
      } else {
        (*map)[*sequence_number] = path;
        temp_file_fd->fd = fd;
        temp_file_fd->auto_close = true;
      }
    }
  }
#elif defined(OS_ANDROID)
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* wc = GetWebContentsForRenderView(render_view_id);
  if (!wc)
    return;
  PrintViewManagerBasic* print_view_manager =
      PrintViewManagerBasic::FromWebContents(wc);
  // The file descriptor is originally created in & passed from the Android
  // side, and it will handle the closing.
  const base::FileDescriptor& file_descriptor =
      print_view_manager->file_descriptor();
  temp_file_fd->fd = file_descriptor.fd;
  temp_file_fd->auto_close = false;
#endif
}

void PrintingMessageFilter::OnTempFileForPrintingWritten(int render_view_id,
                                                         int sequence_number) {
#if defined(OS_CHROMEOS)
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  SequenceToPathMap* map = &g_printing_file_descriptor_map.Get().map;
  SequenceToPathMap::iterator it = map->find(sequence_number);
  if (it == map->end()) {
    NOTREACHED() << "Got a sequence that we didn't pass to the "
                    "renderer: " << sequence_number;
    return;
  }
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PrintingMessageFilter::CreatePrintDialogForFile,
                 this, render_view_id, it->second));

  // Erase the entry in the map.
  map->erase(it);
#elif defined(OS_ANDROID)
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* wc = GetWebContentsForRenderView(render_view_id);
  if (!wc)
    return;
  PrintViewManagerBasic* print_view_manager =
      PrintViewManagerBasic::FromWebContents(wc);
  const base::FileDescriptor& file_descriptor =
      print_view_manager->file_descriptor();
  PrintingContextAndroid::PdfWritingDone(file_descriptor.fd, true);
  // Invalidate the file descriptor so it doesn't accidentally get reused.
  print_view_manager->set_file_descriptor(base::FileDescriptor(-1, false));
#endif
}
#endif  // defined(OS_CHROMEOS) || defined(OS_ANDROID)

#if defined(OS_CHROMEOS)
void PrintingMessageFilter::CreatePrintDialogForFile(
    int render_view_id,
    const base::FilePath& path) {
  content::WebContents* wc = GetWebContentsForRenderView(render_view_id);
  if (!wc)
    return;
  print_dialog_cloud::CreatePrintDialogForFile(
      wc->GetBrowserContext(),
      wc->GetTopLevelNativeWindow(),
      path,
      wc->GetTitle(),
      base::string16(),
      std::string("application/pdf"));
}
#endif  // defined(OS_CHROMEOS)

content::WebContents* PrintingMessageFilter::GetWebContentsForRenderView(
    int render_view_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::RenderViewHost* view = content::RenderViewHost::FromID(
      render_process_id_, render_view_id);
  return view ? content::WebContents::FromRenderViewHost(view) : NULL;
}

void PrintingMessageFilter::OnGetDefaultPrintSettings(IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  scoped_refptr<PrinterQuery> printer_query;
#if 0
  if (!profile_io_data_->printing_enabled()->GetValue()) {
    // Reply with NULL query.
    OnGetDefaultPrintSettingsReply(printer_query, reply_msg);
    return;
  }
#endif
  printer_query = queue_->PopPrinterQuery(0);
  if (!printer_query.get()) {
    printer_query =
        queue_->CreatePrinterQuery(render_process_id_, reply_msg->routing_id());
  }

  // Loads default settings. This is asynchronous, only the IPC message sender
  // will hang until the settings are retrieved.
  printer_query->GetSettings(
      PrinterQuery::DEFAULTS,
      0,
      false,
      DEFAULT_MARGINS,
      base::Bind(&PrintingMessageFilter::OnGetDefaultPrintSettingsReply,
                 this,
                 printer_query,
                 reply_msg));
}

void PrintingMessageFilter::OnGetDefaultPrintSettingsReply(
    scoped_refptr<PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  PrintMsg_Print_Params params;
  if (!printer_query.get() ||
      printer_query->last_status() != PrintingContext::OK) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params);
    params.document_cookie = printer_query->cookie();
  }
  PrintHostMsg_GetDefaultPrintSettings::WriteReplyParams(reply_msg, params);
  Send(reply_msg);
  // If printing was enabled.
  if (printer_query.get()) {
    // If user hasn't cancelled.
    if (printer_query->cookie() && printer_query->settings().dpi()) {
      queue_->QueuePrinterQuery(printer_query.get());
    } else {
      printer_query->StopWorker();
    }
  }
}

void PrintingMessageFilter::OnScriptedPrint(
    const PrintHostMsg_ScriptedPrint_Params& params,
    IPC::Message* reply_msg) {
  scoped_refptr<PrinterQuery> printer_query =
      queue_->PopPrinterQuery(params.cookie);
  if (!printer_query.get()) {
    printer_query =
        queue_->CreatePrinterQuery(render_process_id_, reply_msg->routing_id());
  }
  printer_query->GetSettings(
      PrinterQuery::ASK_USER,
      params.expected_pages_count,
      params.has_selection,
      params.margin_type,
      base::Bind(&PrintingMessageFilter::OnScriptedPrintReply,
                 this,
                 printer_query,
                 reply_msg));
}

void PrintingMessageFilter::OnScriptedPrintReply(
    scoped_refptr<PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  PrintMsg_PrintPages_Params params;
#if defined(OS_ANDROID)
  // We need to save the routing ID here because Send method below deletes the
  // |reply_msg| before we can get the routing ID for the Android code.
  int routing_id = reply_msg->routing_id();
#endif
  if (printer_query->last_status() != PrintingContext::OK ||
      !printer_query->settings().dpi()) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params.params);
    params.params.document_cookie = printer_query->cookie();
    params.pages = PageRange::GetPages(printer_query->settings().ranges());
  }
  PrintHostMsg_ScriptedPrint::WriteReplyParams(reply_msg, params);
  Send(reply_msg);
  if (params.params.dpi && params.params.document_cookie) {
#if defined(OS_ANDROID)
    int file_descriptor;
    const base::string16& device_name = printer_query->settings().device_name();
    if (base::StringToInt(device_name, &file_descriptor)) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&PrintingMessageFilter::UpdateFileDescriptor, this,
                     routing_id, file_descriptor));
    }
#endif
    queue_->QueuePrinterQuery(printer_query.get());
  } else {
    printer_query->StopWorker();
  }
}

#if defined(OS_ANDROID)
void PrintingMessageFilter::UpdateFileDescriptor(int render_view_id, int fd) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* wc = GetWebContentsForRenderView(render_view_id);
  if (!wc)
    return;
  PrintViewManagerBasic* print_view_manager =
      PrintViewManagerBasic::FromWebContents(wc);
  print_view_manager->set_file_descriptor(base::FileDescriptor(fd, false));
}
#endif

void PrintingMessageFilter::OnUpdatePrintSettings(
    int document_cookie, const base::DictionaryValue& job_settings,
    IPC::Message* reply_msg) {
  scoped_ptr<base::DictionaryValue> new_settings(job_settings.DeepCopy());

  scoped_refptr<PrinterQuery> printer_query;
  printer_query = queue_->PopPrinterQuery(document_cookie);
  if (!printer_query.get()) {
    int host_id = render_process_id_;
    int routing_id = reply_msg->routing_id();
    if (!new_settings->GetInteger(printing::kPreviewInitiatorHostId,
                                  &host_id) ||
        !new_settings->GetInteger(printing::kPreviewInitiatorRoutingId,
                                  &routing_id)) {
      host_id = content::ChildProcessHost::kInvalidUniqueID;
      routing_id = content::ChildProcessHost::kInvalidUniqueID;
    }
    printer_query = queue_->CreatePrinterQuery(host_id, routing_id);
  }
  printer_query->SetSettings(
      std::move(new_settings),
      base::Bind(&PrintingMessageFilter::OnUpdatePrintSettingsReply, this,
                 printer_query, reply_msg));
}

void PrintingMessageFilter::OnUpdatePrintSettingsReply(
    scoped_refptr<PrinterQuery> printer_query,
    IPC::Message* reply_msg) {
  PrintMsg_PrintPages_Params params;
  if (!printer_query.get() ||
      printer_query->last_status() != PrintingContext::OK) {
    params.Reset();
  } else {
    RenderParamsFromPrintSettings(printer_query->settings(), &params.params);
    params.params.document_cookie = printer_query->cookie();
    params.pages = PageRange::GetPages(printer_query->settings().ranges());
  }
  PrintHostMsg_UpdatePrintSettings::WriteReplyParams(
      reply_msg,
      params,
      printer_query.get() &&
          (printer_query->last_status() == printing::PrintingContext::CANCEL));
  Send(reply_msg);
  // If user hasn't cancelled.
  if (printer_query.get()) {
    if (printer_query->cookie() && printer_query->settings().dpi()) {
      queue_->QueuePrinterQuery(printer_query.get());
    } else {
      printer_query->StopWorker();
    }
  }
}

}  // namespace printing
