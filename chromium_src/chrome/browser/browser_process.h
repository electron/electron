// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This interface is for managing the global services of the application. Each
// service is lazily created when requested the first time. The service getters
// will return NULL if the service is not available, so callers must check for
// this condition.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_H_
#define CHROME_BROWSER_BROWSER_PROCESS_H_

#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/host_desktop.h"

class BackgroundModeManager;
class CRLSetFetcher;
class DownloadRequestLimiter;
class DownloadStatusUpdater;
class GLStringManager;
class GpuModeManager;
class IconManager;
class IntranetRedirectDetector;
class IOThread;
class MediaFileSystemRegistry;
class NotificationUIManager;
class PrefRegistrySimple;
class PrefService;
class Profile;
class ProfileManager;
class StatusTray;
class WatchDogThread;
#if defined(ENABLE_WEBRTC)
class WebRtcLogUploader;
#endif

namespace safe_browsing {
class SafeBrowsingService;
}

namespace variations {
class VariationsService;
}

namespace component_updater {
class ComponentUpdateService;
class PnaclComponentInstaller;
class SupervisedUserWhitelistInstaller;
}

namespace extensions {
class EventRouterForwarder;
}

namespace gcm {
class GCMDriver;
}

namespace memory {
class TabManager;
}

namespace message_center {
class MessageCenter;
}

namespace metrics {
class MetricsService;
}

namespace metrics_services_manager {
class MetricsServicesManager;
}

namespace net {
class URLRequestContextGetter;
}

namespace net_log {
class ChromeNetLog;
}

namespace network_time {
class NetworkTimeTracker;
}

namespace policy {
class BrowserPolicyConnector;
class PolicyService;
}

namespace printing {
class BackgroundPrintingManager;
class PrintJobManager;
class PrintPreviewDialogController;
}

namespace rappor {
class RapporService;
}

namespace safe_browsing {
class ClientSideDetectionService;
}

namespace web_resource {
class PromoResourceService;
}

// NOT THREAD SAFE, call only from the main thread.
// These functions shouldn't return NULL unless otherwise noted.
class BrowserProcess {
 public:
  BrowserProcess();
  virtual ~BrowserProcess();

  // Called when the ResourceDispatcherHost object is created by content.
  virtual void ResourceDispatcherHostCreated() = 0;

  // Invoked when the user is logging out/shutting down. When logging off we may
  // not have enough time to do a normal shutdown. This method is invoked prior
  // to normal shutdown and saves any state that must be saved before we are
  // continue shutdown.
  virtual void EndSession() = 0;

  // Gets the manager for the various metrics-related services, constructing it
  // if necessary.
  virtual metrics_services_manager::MetricsServicesManager*
  GetMetricsServicesManager() = 0;

  // Services: any of these getters may return NULL
  virtual metrics::MetricsService* metrics_service() = 0;
  virtual rappor::RapporService* rappor_service() = 0;
  virtual ProfileManager* profile_manager() = 0;
  virtual PrefService* local_state() = 0;
  virtual net::URLRequestContextGetter* system_request_context() = 0;
  virtual variations::VariationsService* variations_service() = 0;
  virtual web_resource::PromoResourceService* promo_resource_service() = 0;

  virtual BrowserProcessPlatformPart* platform_part() = 0;

  virtual extensions::EventRouterForwarder*
      extension_event_router_forwarder() = 0;

  // Returns the manager for desktop notifications.
  virtual NotificationUIManager* notification_ui_manager() = 0;

  // MessageCenter is a global list of currently displayed notifications.
  virtual message_center::MessageCenter* message_center() = 0;

  // Returns the state object for the thread that we perform I/O
  // coordination on (network requests, communication with renderers,
  // etc.
  //
  // Can be NULL close to startup and shutdown.
  //
  // NOTE: If you want to post a task to the IO thread, use
  // BrowserThread::PostTask (or other variants).
  virtual IOThread* io_thread() = 0;

  // Returns the thread that is used for health check of all browser threads.
  virtual WatchDogThread* watchdog_thread() = 0;

  // Starts and manages the policy system.
  virtual policy::BrowserPolicyConnector* browser_policy_connector() = 0;

  // This is the main interface for chromium components to retrieve policy
  // information from the policy system.
  virtual policy::PolicyService* policy_service() = 0;

  virtual IconManager* icon_manager() = 0;

  virtual GLStringManager* gl_string_manager() = 0;

  virtual GpuModeManager* gpu_mode_manager() = 0;

  virtual void CreateDevToolsHttpProtocolHandler(
      chrome::HostDesktopType host_desktop_type,
      const std::string& ip,
      uint16_t port) = 0;

  virtual unsigned int AddRefModule() = 0;
  virtual unsigned int ReleaseModule() = 0;

  virtual bool IsShuttingDown() = 0;

  virtual printing::PrintJobManager* print_job_manager() = 0;
  virtual printing::PrintPreviewDialogController*
      print_preview_dialog_controller() = 0;
  virtual printing::BackgroundPrintingManager*
      background_printing_manager() = 0;

  virtual IntranetRedirectDetector* intranet_redirect_detector() = 0;

  // Returns the locale used by the application.
  virtual const std::string& GetApplicationLocale() = 0;
  virtual void SetApplicationLocale(const std::string& locale) = 0;

  virtual DownloadStatusUpdater* download_status_updater() = 0;
  virtual DownloadRequestLimiter* download_request_limiter() = 0;

  // Returns the object that manages background applications.
  virtual BackgroundModeManager* background_mode_manager() = 0;
  virtual void set_background_mode_manager_for_test(
      scoped_ptr<BackgroundModeManager> manager) = 0;

  // Returns the StatusTray, which provides an API for displaying status icons
  // in the system status tray. Returns NULL if status icons are not supported
  // on this platform (or this is a unit test).
  virtual StatusTray* status_tray() = 0;

  // Returns the SafeBrowsing service.
  virtual safe_browsing::SafeBrowsingService* safe_browsing_service() = 0;

  // Returns an object which handles communication with the SafeBrowsing
  // client-side detection servers.
  virtual safe_browsing::ClientSideDetectionService*
      safe_browsing_detection_service() = 0;

#if (defined(OS_WIN) || defined(OS_LINUX)) && !defined(OS_CHROMEOS)
  // This will start a timer that, if Chrome is in persistent mode, will check
  // whether an update is available, and if that's the case, restart the
  // browser. Note that restart code will strip some of the command line keys
  // and all loose values from the cl this instance of Chrome was launched with,
  // and add the command line key that will force Chrome to start in the
  // background mode. For the full list of "blacklisted" keys, refer to
  // |kSwitchesToRemoveOnAutorestart| array in browser_process_impl.cc.
  virtual void StartAutoupdateTimer() = 0;
#endif

  virtual net_log::ChromeNetLog* net_log() = 0;

  virtual component_updater::ComponentUpdateService* component_updater() = 0;

  virtual CRLSetFetcher* crl_set_fetcher() = 0;

  virtual component_updater::PnaclComponentInstaller*
  pnacl_component_installer() = 0;

  virtual component_updater::SupervisedUserWhitelistInstaller*
  supervised_user_whitelist_installer() = 0;

  virtual MediaFileSystemRegistry* media_file_system_registry() = 0;

  virtual bool created_local_state() const = 0;

#if defined(ENABLE_WEBRTC)
  virtual WebRtcLogUploader* webrtc_log_uploader() = 0;
#endif

  virtual network_time::NetworkTimeTracker* network_time_tracker() = 0;

  virtual gcm::GCMDriver* gcm_driver() = 0;

  // Returns the tab manager if it exists, null otherwise.
  virtual memory::TabManager* GetTabManager() = 0;

  // Returns the default web client state of Chrome (i.e., was it the user's
  // default browser) at the time a previous check was made sometime between
  // process startup and now.
  virtual ShellIntegration::DefaultWebClientState
  CachedDefaultWebClientState() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserProcess);
};

extern BrowserProcess* g_browser_process;

#endif  // CHROME_BROWSER_BROWSER_PROCESS_H_
