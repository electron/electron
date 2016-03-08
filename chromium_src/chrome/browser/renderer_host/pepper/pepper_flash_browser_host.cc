// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/pepper/pepper_flash_browser_host.h"

#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "ipc/ipc_message_macros.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/resource_message_params.h"
#include "ppapi/shared_impl/time_conversion.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
#include <CoreServices/CoreServices.h>
#endif

using content::BrowserPpapiHost;
using content::BrowserThread;
using content::RenderProcessHost;

namespace chrome {

namespace {

// Get the CookieSettings on the UI thread for the given render process ID.
scoped_refptr<content_settings::CookieSettings> GetCookieSettings(
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* render_process_host =
      RenderProcessHost::FromID(render_process_id);
  if (render_process_host && render_process_host->GetBrowserContext()) {
    Profile* profile =
        Profile::FromBrowserContext(render_process_host->GetBrowserContext());
    return CookieSettingsFactory::GetForProfile(profile);
  }
  return NULL;
}

}  // namespace

PepperFlashBrowserHost::PepperFlashBrowserHost(BrowserPpapiHost* host,
                                               PP_Instance instance,
                                               PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      host_(host),
      weak_factory_(this) {
  int unused;
  host->GetRenderFrameIDsForInstance(instance, &render_process_id_, &unused);
}

PepperFlashBrowserHost::~PepperFlashBrowserHost() {}

int32_t PepperFlashBrowserHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperFlashBrowserHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_Flash_UpdateActivity,
                                        OnUpdateActivity)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_Flash_GetLocalTimeZoneOffset,
                                      OnGetLocalTimeZoneOffset)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
        PpapiHostMsg_Flash_GetLocalDataRestrictions, OnGetLocalDataRestrictions)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperFlashBrowserHost::OnUpdateActivity(
    ppapi::host::HostMessageContext* host_context) {
#if defined(OS_WIN)
  // Reading then writing back the same value to the screensaver timeout system
  // setting resets the countdown which prevents the screensaver from turning
  // on "for a while". As long as the plugin pings us with this message faster
  // than the screensaver timeout, it won't go on.
  int value = 0;
  if (SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &value, 0))
    SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, value, NULL, 0);
#elif defined(OS_MACOSX)
  UpdateSystemActivity(OverallAct);
#else
// TODO(brettw) implement this for other platforms.
#endif
  return PP_OK;
}

int32_t PepperFlashBrowserHost::OnGetLocalTimeZoneOffset(
    ppapi::host::HostMessageContext* host_context,
    const base::Time& t) {
  // The reason for this processing being in the browser process is that on
  // Linux, the localtime calls require filesystem access prohibited by the
  // sandbox.
  host_context->reply_msg = PpapiPluginMsg_Flash_GetLocalTimeZoneOffsetReply(
      ppapi::PPGetLocalTimeZoneOffset(t));
  return PP_OK;
}

int32_t PepperFlashBrowserHost::OnGetLocalDataRestrictions(
    ppapi::host::HostMessageContext* context) {
  // Getting the Flash LSO settings requires using the CookieSettings which
  // belong to the profile which lives on the UI thread. We lazily initialize
  // |cookie_settings_| by grabbing the reference from the UI thread and then
  // call |GetLocalDataRestrictions| with it.
  GURL document_url = host_->GetDocumentURLForInstance(pp_instance());
  GURL plugin_url = host_->GetPluginURLForInstance(pp_instance());
  if (cookie_settings_.get()) {
    GetLocalDataRestrictions(context->MakeReplyMessageContext(),
                             document_url,
                             plugin_url,
                             cookie_settings_);
  } else {
    BrowserThread::PostTaskAndReplyWithResult(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GetCookieSettings, render_process_id_),
        base::Bind(&PepperFlashBrowserHost::GetLocalDataRestrictions,
                   weak_factory_.GetWeakPtr(),
                   context->MakeReplyMessageContext(),
                   document_url,
                   plugin_url));
  }
  return PP_OK_COMPLETIONPENDING;
}

void PepperFlashBrowserHost::GetLocalDataRestrictions(
    ppapi::host::ReplyMessageContext reply_context,
    const GURL& document_url,
    const GURL& plugin_url,
    scoped_refptr<content_settings::CookieSettings> cookie_settings) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Lazily initialize |cookie_settings_|. The cookie settings are thread-safe
  // ref-counted so as long as we hold a reference to them we can safely access
  // them on the IO thread.
  if (!cookie_settings_.get()) {
    cookie_settings_ = cookie_settings;
  } else {
    DCHECK(cookie_settings_.get() == cookie_settings.get());
  }

  PP_FlashLSORestrictions restrictions = PP_FLASHLSORESTRICTIONS_NONE;
  if (cookie_settings_.get() && document_url.is_valid() &&
      plugin_url.is_valid()) {
    if (!cookie_settings_->IsReadingCookieAllowed(document_url, plugin_url))
      restrictions = PP_FLASHLSORESTRICTIONS_BLOCK;
    else if (cookie_settings_->IsCookieSessionOnly(plugin_url))
      restrictions = PP_FLASHLSORESTRICTIONS_IN_MEMORY;
  }
  SendReply(reply_context,
            PpapiPluginMsg_Flash_GetLocalDataRestrictionsReply(
                static_cast<int32_t>(restrictions)));
}

}  // namespace chrome
