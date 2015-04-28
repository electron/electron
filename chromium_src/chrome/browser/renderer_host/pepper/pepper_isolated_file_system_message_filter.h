// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_ISOLATED_FILE_SYSTEM_MESSAGE_FILTER_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_ISOLATED_FILE_SYSTEM_MESSAGE_FILTER_H_

#include <set>
#include <string>

#include "base/files/file_path.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/private/ppb_isolated_file_system_private.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/host/resource_message_filter.h"
#include "url/gurl.h"

class Profile;

namespace content {
class BrowserPpapiHost;
}

namespace ppapi {
namespace host {
struct HostMessageContext;
}  // namespace host
}  // namespace ppapi

namespace chrome {

class PepperIsolatedFileSystemMessageFilter
    : public ppapi::host::ResourceMessageFilter {
 public:
  static PepperIsolatedFileSystemMessageFilter* Create(
      PP_Instance instance,
      content::BrowserPpapiHost* host);

  // ppapi::host::ResourceMessageFilter implementation.
  scoped_refptr<base::TaskRunner> OverrideTaskRunnerForMessage(
      const IPC::Message& msg) override;
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  PepperIsolatedFileSystemMessageFilter(int render_process_id,
                                        const base::FilePath& profile_directory,
                                        const GURL& document_url,
                                        ppapi::host::PpapiHost* ppapi_host_);

  ~PepperIsolatedFileSystemMessageFilter() override;

  // Returns filesystem id of isolated filesystem if valid, or empty string
  // otherwise.  This must run on the UI thread because ProfileManager only
  // allows access on that thread.

  int32_t OnOpenFileSystem(ppapi::host::HostMessageContext* context,
                           PP_IsolatedFileSystemType_Private type);
  int32_t OpenPluginPrivateFileSystem(ppapi::host::HostMessageContext* context);

  const int render_process_id_;
  // Keep a copy from original thread.
  const base::FilePath profile_directory_;
  const GURL document_url_;

  // Not owned by this object.
  ppapi::host::PpapiHost* ppapi_host_;

  DISALLOW_COPY_AND_ASSIGN(PepperIsolatedFileSystemMessageFilter);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_PEPPER_ISOLATED_FILE_SYSTEM_MESSAGE_FILTER_H_
