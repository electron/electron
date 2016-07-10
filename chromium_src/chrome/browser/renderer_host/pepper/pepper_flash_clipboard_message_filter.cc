// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/pepper/pepper_flash_clipboard_message_filter.h"

#include <stddef.h>

#include "base/pickle.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash_clipboard.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_message_params.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

using content::BrowserThread;

namespace chrome {

namespace {

const size_t kMaxClipboardWriteSize = 1000000;

ui::ClipboardType ConvertClipboardType(uint32_t type) {
  switch (type) {
    case PP_FLASH_CLIPBOARD_TYPE_STANDARD:
      return ui::CLIPBOARD_TYPE_COPY_PASTE;
    case PP_FLASH_CLIPBOARD_TYPE_SELECTION:
      return ui::CLIPBOARD_TYPE_SELECTION;
  }
  NOTREACHED();
  return ui::CLIPBOARD_TYPE_COPY_PASTE;
}

// Functions to pack/unpack custom data from a pickle. See the header file for
// more detail on custom formats in Pepper.
// TODO(raymes): Currently pepper custom formats are stored in their own
// native format type. However we should be able to store them in the same way
// as "Web Custom" formats are. This would allow clipboard data to be shared
// between pepper applications and web applications. However currently web apps
// assume all data that is placed on the clipboard is UTF16 and pepper allows
// arbitrary data so this change would require some reworking of the chrome
// clipboard interface for custom data.
bool JumpToFormatInPickle(const base::string16& format,
                          base::PickleIterator* iter) {
  uint32_t size = 0;
  if (!iter->ReadUInt32(&size))
    return false;
  for (uint32_t i = 0; i < size; ++i) {
    base::string16 stored_format;
    if (!iter->ReadString16(&stored_format))
      return false;
    if (stored_format == format)
      return true;
    int skip_length;
    if (!iter->ReadLength(&skip_length))
      return false;
    if (!iter->SkipBytes(skip_length))
      return false;
  }
  return false;
}

bool IsFormatAvailableInPickle(const base::string16& format,
                               const base::Pickle& pickle) {
  base::PickleIterator iter(pickle);
  return JumpToFormatInPickle(format, &iter);
}

std::string ReadDataFromPickle(const base::string16& format,
                               const base::Pickle& pickle) {
  std::string result;
  base::PickleIterator iter(pickle);
  if (!JumpToFormatInPickle(format, &iter) || !iter.ReadString(&result))
    return std::string();
  return result;
}

bool WriteDataToPickle(const std::map<base::string16, std::string>& data,
                       base::Pickle* pickle) {
  pickle->WriteUInt32(data.size());
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (!pickle->WriteString16(it->first))
      return false;
    if (!pickle->WriteString(it->second))
      return false;
  }
  return true;
}

}  // namespace

PepperFlashClipboardMessageFilter::PepperFlashClipboardMessageFilter() {}

PepperFlashClipboardMessageFilter::~PepperFlashClipboardMessageFilter() {}

scoped_refptr<base::TaskRunner>
PepperFlashClipboardMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& msg) {
  // Clipboard writes should always occur on the UI thread due to the
  // restrictions of various platform APIs. In general, the clipboard is not
  // thread-safe, so all clipboard calls should be serviced from the UI thread.
  if (msg.type() == PpapiHostMsg_FlashClipboard_WriteData::ID)
    return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI);

// Windows needs clipboard reads to be serviced from the IO thread because
// these are sync IPCs which can result in deadlocks with plugins if serviced
// from the UI thread. Note that Windows clipboard calls ARE thread-safe so it
// is ok for reads and writes to be serviced from different threads.
#if !defined(OS_WIN)
  return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI);
#else
  return BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO);
#endif
}

int32_t PepperFlashClipboardMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashClipboardMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_FlashClipboard_RegisterCustomFormat,
        OnMsgRegisterCustomFormat)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_FlashClipboard_IsFormatAvailable, OnMsgIsFormatAvailable)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FlashClipboard_ReadData,
                                      OnMsgReadData)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_FlashClipboard_WriteData,
                                      OnMsgWriteData)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_FlashClipboard_GetSequenceNumber, OnMsgGetSequenceNumber)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashClipboardMessageFilter::OnMsgRegisterCustomFormat(
    ppapi::host::HostMessageContext* host_context,
    const std::string& format_name) {
  uint32_t format = custom_formats_.RegisterFormat(format_name);
  if (format == PP_FLASH_CLIPBOARD_FORMAT_INVALID)
    return PP_ERROR_FAILED;
  host_context->reply_msg =
      PpapiPluginMsg_FlashClipboard_RegisterCustomFormatReply(format);
  return PP_OK;
}

int32_t PepperFlashClipboardMessageFilter::OnMsgIsFormatAvailable(
    ppapi::host::HostMessageContext* host_context,
    uint32_t clipboard_type,
    uint32_t format) {
  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_ERROR_FAILED;
  }

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardType type = ConvertClipboardType(clipboard_type);
  bool available = false;
  switch (format) {
    case PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT: {
      bool plain = clipboard->IsFormatAvailable(
          ui::Clipboard::GetPlainTextFormatType(), type);
      bool plainw = clipboard->IsFormatAvailable(
          ui::Clipboard::GetPlainTextWFormatType(), type);
      available = plain || plainw;
      break;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_HTML:
      available = clipboard->IsFormatAvailable(
          ui::Clipboard::GetHtmlFormatType(), type);
      break;
    case PP_FLASH_CLIPBOARD_FORMAT_RTF:
      available =
          clipboard->IsFormatAvailable(ui::Clipboard::GetRtfFormatType(), type);
      break;
    case PP_FLASH_CLIPBOARD_FORMAT_INVALID:
      break;
    default:
      if (custom_formats_.IsFormatRegistered(format)) {
        std::string format_name = custom_formats_.GetFormatName(format);
        std::string clipboard_data;
        clipboard->ReadData(ui::Clipboard::GetPepperCustomDataFormatType(),
                            &clipboard_data);
        base::Pickle pickle(clipboard_data.data(), clipboard_data.size());
        available =
            IsFormatAvailableInPickle(base::UTF8ToUTF16(format_name), pickle);
      }
      break;
  }

  return available ? PP_OK : PP_ERROR_FAILED;
}

int32_t PepperFlashClipboardMessageFilter::OnMsgReadData(
    ppapi::host::HostMessageContext* host_context,
    uint32_t clipboard_type,
    uint32_t format) {
  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_ERROR_FAILED;
  }

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardType type = ConvertClipboardType(clipboard_type);
  std::string clipboard_string;
  int32_t result = PP_ERROR_FAILED;
  switch (format) {
    case PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT: {
      if (clipboard->IsFormatAvailable(ui::Clipboard::GetPlainTextWFormatType(),
                                       type)) {
        base::string16 text;
        clipboard->ReadText(type, &text);
        if (!text.empty()) {
          result = PP_OK;
          clipboard_string = base::UTF16ToUTF8(text);
          break;
        }
      }
      // If the PlainTextW format isn't available or is empty, take the
      // ASCII text format.
      if (clipboard->IsFormatAvailable(ui::Clipboard::GetPlainTextFormatType(),
                                       type)) {
        result = PP_OK;
        clipboard->ReadAsciiText(type, &clipboard_string);
      }
      break;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_HTML: {
      if (!clipboard->IsFormatAvailable(ui::Clipboard::GetHtmlFormatType(),
                                        type)) {
        break;
      }

      base::string16 html;
      std::string url;
      uint32_t fragment_start;
      uint32_t fragment_end;
      clipboard->ReadHTML(type, &html, &url, &fragment_start, &fragment_end);
      result = PP_OK;
      clipboard_string = base::UTF16ToUTF8(
          html.substr(fragment_start, fragment_end - fragment_start));
      break;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_RTF: {
      if (!clipboard->IsFormatAvailable(ui::Clipboard::GetRtfFormatType(),
                                        type)) {
        break;
      }
      result = PP_OK;
      clipboard->ReadRTF(type, &clipboard_string);
      break;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_INVALID:
      break;
    default: {
      if (custom_formats_.IsFormatRegistered(format)) {
        base::string16 format_name =
            base::UTF8ToUTF16(custom_formats_.GetFormatName(format));
        std::string clipboard_data;
        clipboard->ReadData(ui::Clipboard::GetPepperCustomDataFormatType(),
                            &clipboard_data);
        base::Pickle pickle(clipboard_data.data(), clipboard_data.size());
        if (IsFormatAvailableInPickle(format_name, pickle)) {
          result = PP_OK;
          clipboard_string = ReadDataFromPickle(format_name, pickle);
        }
      }
      break;
    }
  }

  if (result == PP_OK) {
    host_context->reply_msg =
        PpapiPluginMsg_FlashClipboard_ReadDataReply(clipboard_string);
  }
  return result;
}

int32_t PepperFlashClipboardMessageFilter::OnMsgWriteData(
    ppapi::host::HostMessageContext* host_context,
    uint32_t clipboard_type,
    const std::vector<uint32_t>& formats,
    const std::vector<std::string>& data) {
  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_ERROR_FAILED;
  }
  if (formats.size() != data.size())
    return PP_ERROR_FAILED;

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardType type = ConvertClipboardType(clipboard_type);
  // If no formats are passed in clear the clipboard.
  if (formats.size() == 0) {
    clipboard->Clear(type);
    return PP_OK;
  }

  ui::ScopedClipboardWriter scw(type);
  std::map<base::string16, std::string> custom_data_map;
  int32_t res = PP_OK;
  for (uint32_t i = 0; i < formats.size(); ++i) {
    if (data[i].length() > kMaxClipboardWriteSize) {
      res = PP_ERROR_NOSPACE;
      break;
    }

    switch (formats[i]) {
      case PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT:
        scw.WriteText(base::UTF8ToUTF16(data[i]));
        break;
      case PP_FLASH_CLIPBOARD_FORMAT_HTML:
        scw.WriteHTML(base::UTF8ToUTF16(data[i]), std::string());
        break;
      case PP_FLASH_CLIPBOARD_FORMAT_RTF:
        scw.WriteRTF(data[i]);
        break;
      case PP_FLASH_CLIPBOARD_FORMAT_INVALID:
        res = PP_ERROR_BADARGUMENT;
        break;
      default:
        if (custom_formats_.IsFormatRegistered(formats[i])) {
          std::string format_name = custom_formats_.GetFormatName(formats[i]);
          custom_data_map[base::UTF8ToUTF16(format_name)] = data[i];
        } else {
          // Invalid format.
          res = PP_ERROR_BADARGUMENT;
          break;
        }
    }

    if (res != PP_OK)
      break;
  }

  if (custom_data_map.size() > 0) {
    base::Pickle pickle;
    if (WriteDataToPickle(custom_data_map, &pickle)) {
      scw.WritePickledData(pickle,
                           ui::Clipboard::GetPepperCustomDataFormatType());
    } else {
      res = PP_ERROR_BADARGUMENT;
    }
  }

  if (res != PP_OK) {
    // Need to clear the objects so nothing is written.
    scw.Reset();
  }

  return res;
}

int32_t PepperFlashClipboardMessageFilter::OnMsgGetSequenceNumber(
    ppapi::host::HostMessageContext* host_context,
    uint32_t clipboard_type) {
  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_ERROR_FAILED;
  }

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardType type = ConvertClipboardType(clipboard_type);
  int64_t sequence_number = clipboard->GetSequenceNumber(type);
  host_context->reply_msg =
      PpapiPluginMsg_FlashClipboard_GetSequenceNumberReply(sequence_number);
  return PP_OK;
}

}  // namespace chrome
