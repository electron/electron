// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/pepper/pepper_flash_drm_renderer_host.h"

#include "base/files/file_path.h"
#include "content/public/renderer/pepper_plugin_instance.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"

// TODO(raymes): This is duplicated from pepper_flash_drm_host.cc but once
// FileRef is refactored to the browser, it won't need to be.
namespace {
const char kVoucherFilename[] = "plugin.vch";
}  // namespace

PepperFlashDRMRendererHost::PepperFlashDRMRendererHost(
    content::RendererPpapiHost* host,
    PP_Instance instance,
    PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      renderer_ppapi_host_(host),
      weak_factory_(this) {}

PepperFlashDRMRendererHost::~PepperFlashDRMRendererHost() {}

int32_t PepperFlashDRMRendererHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashDRMRendererHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_FlashDRM_GetVoucherFile,
                                        OnGetVoucherFile)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashDRMRendererHost::OnGetVoucherFile(
    ppapi::host::HostMessageContext* context) {
  content::PepperPluginInstance* plugin_instance =
      renderer_ppapi_host_->GetPluginInstance(pp_instance());
  if (!plugin_instance)
    return PP_ERROR_FAILED;

  base::FilePath plugin_dir = plugin_instance->GetModulePath().DirName();
  DCHECK(!plugin_dir.empty());
  base::FilePath voucher_file = plugin_dir.AppendASCII(kVoucherFilename);

  int renderer_pending_host_id =
      plugin_instance->MakePendingFileRefRendererHost(voucher_file);
  if (renderer_pending_host_id == 0)
    return PP_ERROR_FAILED;

  std::vector<IPC::Message> create_msgs;
  create_msgs.push_back(PpapiHostMsg_FileRef_CreateForRawFS(voucher_file));

  renderer_ppapi_host_->CreateBrowserResourceHosts(
      pp_instance(),
      create_msgs,
      base::Bind(&PepperFlashDRMRendererHost::DidCreateFileRefHosts,
                 weak_factory_.GetWeakPtr(),
                 context->MakeReplyMessageContext(),
                 voucher_file,
                 renderer_pending_host_id));
  return PP_OK_COMPLETIONPENDING;
}

void PepperFlashDRMRendererHost::DidCreateFileRefHosts(
    const ppapi::host::ReplyMessageContext& reply_context,
    const base::FilePath& external_path,
    int renderer_pending_host_id,
    const std::vector<int>& browser_pending_host_ids) {
  DCHECK_EQ(1U, browser_pending_host_ids.size());
  int browser_pending_host_id = browser_pending_host_ids[0];

  ppapi::FileRefCreateInfo create_info =
      ppapi::MakeExternalFileRefCreateInfo(external_path,
                                           std::string(),
                                           browser_pending_host_id,
                                           renderer_pending_host_id);
  host()->SendReply(reply_context,
                    PpapiPluginMsg_FlashDRM_GetVoucherFileReply(create_info));
}
