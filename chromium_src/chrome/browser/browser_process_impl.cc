// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_impl.h"

#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "chrome/browser/printing/print_job_manager.h"
#endif

BrowserProcessImpl::BrowserProcessImpl() {
  g_browser_process = this;
}

BrowserProcessImpl::~BrowserProcessImpl() {
  g_browser_process = nullptr;
}

bool BrowserProcessImpl::IsShuttingDown() {
  return false;
}

metrics_services_manager::MetricsServicesManager*
BrowserProcessImpl::GetMetricsServicesManager() {
  return nullptr;
}

metrics::MetricsService* BrowserProcessImpl::metrics_service() {
  return nullptr;
}

rappor::RapporServiceImpl* BrowserProcessImpl::rappor_service() {
  return nullptr;
}

ProfileManager* BrowserProcessImpl::profile_manager() {
  return nullptr;
}

PrefService* BrowserProcessImpl::local_state() {
  return nullptr;
}

net::URLRequestContextGetter* BrowserProcessImpl::system_request_context() {
  return nullptr;
}

scoped_refptr<network::SharedURLLoaderFactory>
BrowserProcessImpl::shared_url_loader_factory() {
  return nullptr;
}

variations::VariationsService* BrowserProcessImpl::variations_service() {
  return nullptr;
}

BrowserProcessPlatformPart* BrowserProcessImpl::platform_part() {
  return nullptr;
}

extensions::EventRouterForwarder*
BrowserProcessImpl::extension_event_router_forwarder() {
  return nullptr;
}

NotificationUIManager* BrowserProcessImpl::notification_ui_manager() {
  return nullptr;
}

NotificationPlatformBridge* BrowserProcessImpl::notification_platform_bridge() {
  return nullptr;
}

IOThread* BrowserProcessImpl::io_thread() {
  return nullptr;
}

SystemNetworkContextManager*
BrowserProcessImpl::system_network_context_manager() {
  return nullptr;
}

network::NetworkQualityTracker* BrowserProcessImpl::network_quality_tracker() {
  return nullptr;
}

WatchDogThread* BrowserProcessImpl::watchdog_thread() {
  return nullptr;
}

policy::ChromeBrowserPolicyConnector*
BrowserProcessImpl::browser_policy_connector() {
  return nullptr;
}

policy::PolicyService* BrowserProcessImpl::policy_service() {
  return nullptr;
}

IconManager* BrowserProcessImpl::icon_manager() {
  return nullptr;
}

GpuModeManager* BrowserProcessImpl::gpu_mode_manager() {
  return nullptr;
}

printing::PrintPreviewDialogController*
BrowserProcessImpl::print_preview_dialog_controller() {
  return nullptr;
}

printing::BackgroundPrintingManager*
BrowserProcessImpl::background_printing_manager() {
  return nullptr;
}

IntranetRedirectDetector* BrowserProcessImpl::intranet_redirect_detector() {
  return nullptr;
}

DownloadStatusUpdater* BrowserProcessImpl::download_status_updater() {
  return nullptr;
}

DownloadRequestLimiter* BrowserProcessImpl::download_request_limiter() {
  return nullptr;
}

BackgroundModeManager* BrowserProcessImpl::background_mode_manager() {
  return nullptr;
}

StatusTray* BrowserProcessImpl::status_tray() {
  return nullptr;
}

safe_browsing::SafeBrowsingService*
BrowserProcessImpl::safe_browsing_service() {
  return nullptr;
}

safe_browsing::ClientSideDetectionService*
BrowserProcessImpl::safe_browsing_detection_service() {
  return nullptr;
}

subresource_filter::ContentRulesetService*
BrowserProcessImpl::subresource_filter_ruleset_service() {
  return nullptr;
}

optimization_guide::OptimizationGuideService*
BrowserProcessImpl::optimization_guide_service() {
  return nullptr;
}

net_log::ChromeNetLog* BrowserProcessImpl::net_log() {
  return nullptr;
}

component_updater::ComponentUpdateService*
BrowserProcessImpl::component_updater() {
  return nullptr;
}

component_updater::SupervisedUserWhitelistInstaller*
BrowserProcessImpl::supervised_user_whitelist_installer() {
  return nullptr;
}

MediaFileSystemRegistry* BrowserProcessImpl::media_file_system_registry() {
  return nullptr;
}

WebRtcLogUploader* BrowserProcessImpl::webrtc_log_uploader() {
  return nullptr;
}

network_time::NetworkTimeTracker* BrowserProcessImpl::network_time_tracker() {
  return nullptr;
}

gcm::GCMDriver* BrowserProcessImpl::gcm_driver() {
  return nullptr;
}

resource_coordinator::TabManager* BrowserProcessImpl::GetTabManager() {
  return nullptr;
}

shell_integration::DefaultWebClientState
BrowserProcessImpl::CachedDefaultWebClientState() {
  return shell_integration::UNKNOWN_DEFAULT;
}

prefs::InProcessPrefServiceFactory* BrowserProcessImpl::pref_service_factory()
    const {
  return nullptr;
}

content::NetworkConnectionTracker*
BrowserProcessImpl::network_connection_tracker() {
  return nullptr;
}

void BrowserProcessImpl::SetApplicationLocale(const std::string& locale) {
  locale_ = locale;
}

const std::string& BrowserProcessImpl::GetApplicationLocale() {
  return locale_;
}

printing::PrintJobManager* BrowserProcessImpl::print_job_manager() {
#if BUILDFLAG(ENABLE_PRINTING)
  if (!print_job_manager_)
    print_job_manager_.reset(new printing::PrintJobManager());
  return print_job_manager_.get();
#else
  return nullptr;
#endif
}
