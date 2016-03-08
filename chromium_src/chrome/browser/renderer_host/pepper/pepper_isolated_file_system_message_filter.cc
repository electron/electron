// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/pepper/pepper_isolated_file_system_message_filter.h"

#include <stddef.h>

#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pepper_permission_util.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_view_host.h"
#if defined(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#endif
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/file_system_util.h"
#include "storage/browser/fileapi/isolated_context.h"

namespace chrome {

namespace {

const char* kPredefinedAllowedCrxFsOrigins[] = {
    "6EAED1924DB611B6EEF2A664BD077BE7EAD33B8F",  // see crbug.com/234789
    "4EB74897CB187C7633357C2FE832E0AD6A44883A"   // see crbug.com/234789
};

}  // namespace

// static
PepperIsolatedFileSystemMessageFilter*
PepperIsolatedFileSystemMessageFilter::Create(PP_Instance instance,
                                              content::BrowserPpapiHost* host) {
  int render_process_id;
  int unused_render_frame_id;
  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id, &unused_render_frame_id)) {
    return NULL;
  }
  return new PepperIsolatedFileSystemMessageFilter(
      render_process_id,
      host->GetProfileDataDirectory(),
      host->GetDocumentURLForInstance(instance),
      host->GetPpapiHost());
}

PepperIsolatedFileSystemMessageFilter::PepperIsolatedFileSystemMessageFilter(
    int render_process_id,
    const base::FilePath& profile_directory,
    const GURL& document_url,
    ppapi::host::PpapiHost* ppapi_host)
    : render_process_id_(render_process_id),
      profile_directory_(profile_directory),
      document_url_(document_url),
      ppapi_host_(ppapi_host) {
  for (size_t i = 0; i < arraysize(kPredefinedAllowedCrxFsOrigins); ++i)
    allowed_crxfs_origins_.insert(kPredefinedAllowedCrxFsOrigins[i]);
}

PepperIsolatedFileSystemMessageFilter::
    ~PepperIsolatedFileSystemMessageFilter() {}

scoped_refptr<base::TaskRunner>
PepperIsolatedFileSystemMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& msg) {
  // In order to reach ExtensionSystem, we need to get ProfileManager first.
  // ProfileManager lives in UI thread, so we need to do this in UI thread.
  return content::BrowserThread::GetMessageLoopProxyForThread(
      content::BrowserThread::UI);
}

int32_t PepperIsolatedFileSystemMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperIsolatedFileSystemMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_IsolatedFileSystem_BrowserOpen,
      OnOpenFileSystem)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

Profile* PepperIsolatedFileSystemMessageFilter::GetProfile() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  return profile_manager->GetProfile(profile_directory_);
}

std::string PepperIsolatedFileSystemMessageFilter::CreateCrxFileSystem(
    Profile* profile) {
#if defined(ENABLE_EXTENSIONS)
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile)->enabled_extensions().GetByID(
          document_url_.host());
  if (!extension)
    return std::string();

  // First level directory for isolated filesystem to lookup.
  std::string kFirstLevelDirectory("crxfs");
  return storage::IsolatedContext::GetInstance()->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal,
      std::string(),
      extension->path(),
      &kFirstLevelDirectory);
#else
  return std::string();
#endif
}

int32_t PepperIsolatedFileSystemMessageFilter::OnOpenFileSystem(
    ppapi::host::HostMessageContext* context,
    PP_IsolatedFileSystemType_Private type) {
  switch (type) {
    case PP_ISOLATEDFILESYSTEMTYPE_PRIVATE_INVALID:
      break;
    case PP_ISOLATEDFILESYSTEMTYPE_PRIVATE_CRX:
      return OpenCrxFileSystem(context);
    case PP_ISOLATEDFILESYSTEMTYPE_PRIVATE_PLUGINPRIVATE:
      return OpenPluginPrivateFileSystem(context);
  }
  NOTREACHED();
  context->reply_msg =
      PpapiPluginMsg_IsolatedFileSystem_BrowserOpenReply(std::string());
  return PP_ERROR_FAILED;
}

int32_t PepperIsolatedFileSystemMessageFilter::OpenCrxFileSystem(
    ppapi::host::HostMessageContext* context) {
#if defined(ENABLE_EXTENSIONS)
  Profile* profile = GetProfile();
  const extensions::ExtensionSet* extension_set = NULL;
  if (profile) {
    extension_set =
        &extensions::ExtensionRegistry::Get(profile)->enabled_extensions();
  }
  if (!IsExtensionOrSharedModuleWhitelisted(
          document_url_, extension_set, allowed_crxfs_origins_) &&
      !IsHostAllowedByCommandLine(
          document_url_, extension_set, switches::kAllowNaClCrxFsAPI)) {
    LOG(ERROR) << "Host " << document_url_.host() << " cannot use CrxFs API.";
    return PP_ERROR_NOACCESS;
  }

  // TODO(raymes): When we remove FileSystem from the renderer, we should create
  // a pending PepperFileSystemBrowserHost here with the fsid and send the
  // pending host ID back to the plugin.
  const std::string fsid = CreateCrxFileSystem(profile);
  if (fsid.empty()) {
    context->reply_msg =
        PpapiPluginMsg_IsolatedFileSystem_BrowserOpenReply(std::string());
    return PP_ERROR_NOTSUPPORTED;
  }

  // Grant readonly access of isolated filesystem to renderer process.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantReadFileSystem(render_process_id_, fsid);

  context->reply_msg = PpapiPluginMsg_IsolatedFileSystem_BrowserOpenReply(fsid);
  return PP_OK;
#else
  return PP_ERROR_NOTSUPPORTED;
#endif
}

int32_t PepperIsolatedFileSystemMessageFilter::OpenPluginPrivateFileSystem(
    ppapi::host::HostMessageContext* context) {
  DCHECK(ppapi_host_);
  // Only plugins with private permission can open the filesystem.
  if (!ppapi_host_->permissions().HasPermission(ppapi::PERMISSION_PRIVATE))
    return PP_ERROR_NOACCESS;

  const std::string& root_name = ppapi::IsolatedFileSystemTypeToRootName(
      PP_ISOLATEDFILESYSTEMTYPE_PRIVATE_PLUGINPRIVATE);
  const std::string& fsid =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForVirtualPath(
          storage::kFileSystemTypePluginPrivate, root_name, base::FilePath());

  // Grant full access of isolated filesystem to renderer process.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantCreateReadWriteFileSystem(render_process_id_, fsid);

  context->reply_msg = PpapiPluginMsg_IsolatedFileSystem_BrowserOpenReply(fsid);
  return PP_OK;
}

}  // namespace chrome
