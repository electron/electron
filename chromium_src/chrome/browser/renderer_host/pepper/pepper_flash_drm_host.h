// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_FLASH_DRM_HOST_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_FLASH_DRM_HOST_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/renderer_host/pepper/device_id_fetcher.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"

namespace content {
class BrowserPpapiHost;
}

namespace IPC {
class Message;
}

namespace chrome {
class MonitorFinder;

class PepperFlashDRMHost : public ppapi::host::ResourceHost {
 public:
  PepperFlashDRMHost(content::BrowserPpapiHost* host,
                     PP_Instance instance,
                     PP_Resource resource);
  ~PepperFlashDRMHost() override;

  // ResourceHost override.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  // IPC message handler.
  int32_t OnHostMsgGetDeviceID(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgGetHmonitor(ppapi::host::HostMessageContext* context);
  int32_t OnHostMsgMonitorIsExternal(ppapi::host::HostMessageContext* context);

  // Called by the fetcher when the device ID was retrieved, or the empty string
  // on error.
  void GotDeviceID(ppapi::host::ReplyMessageContext reply_context,
                   const std::string& id,
                   int32_t result);

  scoped_refptr<DeviceIDFetcher> fetcher_;
  scoped_refptr<MonitorFinder> monitor_finder_;

  base::WeakPtrFactory<PepperFlashDRMHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperFlashDRMHost);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_FLASH_DRM_HOST_H_
