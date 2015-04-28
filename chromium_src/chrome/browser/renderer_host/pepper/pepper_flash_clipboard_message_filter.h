// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_FLASH_CLIPBOARD_MESSAGE_FILTER_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_FLASH_CLIPBOARD_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ppapi/host/resource_message_filter.h"
#include "ppapi/shared_impl/flash_clipboard_format_registry.h"

namespace ppapi {
namespace host {
struct HostMessageContext;
}
}

namespace ui {
class ScopedClipboardWriter;
}

namespace chrome {

// Resource message filter for accessing the clipboard in Pepper. Pepper
// supports reading/writing custom formats from the clipboard. Currently, all
// custom formats that are read/written from the clipboard through pepper are
// stored in a single real clipboard format (in the same way the "web custom"
// clipboard formats are). This is done so that we don't have to have use real
// clipboard types for each custom clipboard format which may be a limited
// resource on a particular platform.
class PepperFlashClipboardMessageFilter
    : public ppapi::host::ResourceMessageFilter {
 public:
  PepperFlashClipboardMessageFilter();

 protected:
  // ppapi::host::ResourceMessageFilter overrides.
  scoped_refptr<base::TaskRunner> OverrideTaskRunnerForMessage(
      const IPC::Message& msg) override;
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  ~PepperFlashClipboardMessageFilter() override;

  int32_t OnMsgRegisterCustomFormat(
      ppapi::host::HostMessageContext* host_context,
      const std::string& format_name);
  int32_t OnMsgIsFormatAvailable(ppapi::host::HostMessageContext* host_context,
                                 uint32_t clipboard_type,
                                 uint32_t format);
  int32_t OnMsgReadData(ppapi::host::HostMessageContext* host_context,
                        uint32_t clipoard_type,
                        uint32_t format);
  int32_t OnMsgWriteData(ppapi::host::HostMessageContext* host_context,
                         uint32_t clipboard_type,
                         const std::vector<uint32_t>& formats,
                         const std::vector<std::string>& data);
  int32_t OnMsgGetSequenceNumber(ppapi::host::HostMessageContext* host_context,
                                 uint32_t clipboard_type);

  int32_t WriteClipboardDataItem(uint32_t format,
                                 const std::string& data,
                                 ui::ScopedClipboardWriter* scw);

  ppapi::FlashClipboardFormatRegistry custom_formats_;

  DISALLOW_COPY_AND_ASSIGN(PepperFlashClipboardMessageFilter);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_FLASH_CLIPBOARD_MESSAGE_FILTER_H_
