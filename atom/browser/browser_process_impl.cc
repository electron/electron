// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/browser_process_impl.h"

#include <utility>

#include "chrome/browser/net/chrome_net_log_helper.h"
#include "chrome/common/chrome_switches.h"
#include "components/net_log/chrome_net_log.h"
#include "components/net_log/net_export_file_writer.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/common/content_switches.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "services/network/public/cpp/network_switches.h"
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

// static
void BrowserProcessImpl::ApplyProxyModeFromCommandLine(
    ValueMapPrefStore* pref_store) {
  if (!pref_store)
    return;

  auto* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(switches::kNoProxyServer)) {
    pref_store->SetValue(
        proxy_config::prefs::kProxy,
        std::make_unique<base::Value>(ProxyConfigDictionary::CreateDirect()),
        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else if (command_line->HasSwitch(switches::kProxyPacUrl)) {
    std::string pac_script_url =
        command_line->GetSwitchValueASCII(switches::kProxyPacUrl);
    pref_store->SetValue(
        proxy_config::prefs::kProxy,
        std::make_unique<base::Value>(ProxyConfigDictionary::CreatePacScript(
            pac_script_url, false /* pac_mandatory */)),
        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else if (command_line->HasSwitch(switches::kProxyAutoDetect)) {
    pref_store->SetValue(proxy_config::prefs::kProxy,
                         std::make_unique<base::Value>(
                             ProxyConfigDictionary::CreateAutoDetect()),
                         WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  } else if (command_line->HasSwitch(switches::kProxyServer)) {
    std::string proxy_server =
        command_line->GetSwitchValueASCII(switches::kProxyServer);
    std::string bypass_list =
        command_line->GetSwitchValueASCII(switches::kProxyBypassList);
    pref_store->SetValue(
        proxy_config::prefs::kProxy,
        std::make_unique<base::Value>(ProxyConfigDictionary::CreateFixedServers(
            proxy_server, bypass_list)),
        WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  }
}

void BrowserProcessImpl::PostEarlyInitialization() {
  // Mock user prefs, as we only need to track changes for a
  // in memory pref store. There are no persistent preferences
  PrefServiceFactory prefs_factory;
  auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();
  PrefProxyConfigTrackerImpl::RegisterPrefs(pref_registry.get());
  auto pref_store = base::MakeRefCounted<ValueMapPrefStore>();
  ApplyProxyModeFromCommandLine(pref_store.get());
  prefs_factory.set_command_line_prefs(std::move(pref_store));
  prefs_factory.set_user_prefs(new OverlayUserPrefStore(new InMemoryPrefStore));
  local_state_ = prefs_factory.Create(std::move(pref_registry));
}

void BrowserProcessImpl::PreCreateThreads(
    const base::CommandLine& command_line) {
  // Must be created before the IOThread.
  // Once IOThread class is no longer needed,
  // this can be created on first use.
  system_network_context_manager_ =
      std::make_unique<SystemNetworkContextManager>();

  net_log_ = std::make_unique<net_log::ChromeNetLog>();
  // start net log trace if --log-net-log is passed in the command line.
  if (command_line.HasSwitch(network::switches::kLogNetLog)) {
    base::FilePath log_file =
        command_line.GetSwitchValuePath(network::switches::kLogNetLog);
    if (!log_file.empty()) {
      net_log_->StartWritingToFile(
          log_file, GetNetCaptureModeFromCommandLine(command_line),
          command_line.GetCommandLineString(), std::string());
    }
  }
  // Initialize net log file exporter.
  system_network_context_manager_->GetNetExportFileWriter()->Initialize();

  // Manage global state of net and other IO thread related.
  io_thread_ = std::make_unique<IOThread>(
      net_log_.get(), system_network_context_manager_.get());
}

void BrowserProcessImpl::PostDestroyThreads() {
  io_thread_.reset();
}

void BrowserProcessImpl::PostMainMessageLoopRun() {
  // This expects to be destroyed before the task scheduler is torn down.
  system_network_context_manager_.reset();
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
  DCHECK(local_state_.get());
  return local_state_.get();
}

net::URLRequestContextGetter* BrowserProcessImpl::system_request_context() {
  return nullptr;
}

scoped_refptr<network::SharedURLLoaderFactory>
BrowserProcessImpl::shared_url_loader_factory() {
  return system_network_context_manager()->GetSharedURLLoaderFactory();
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
  DCHECK(io_thread_.get());
  return io_thread_.get();
}

SystemNetworkContextManager*
BrowserProcessImpl::system_network_context_manager() {
  DCHECK(system_network_context_manager_.get());
  return system_network_context_manager_.get();
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

subresource_filter::RulesetService*
BrowserProcessImpl::subresource_filter_ruleset_service() {
  return nullptr;
}

optimization_guide::OptimizationGuideService*
BrowserProcessImpl::optimization_guide_service() {
  return nullptr;
}

net_log::ChromeNetLog* BrowserProcessImpl::net_log() {
  DCHECK(net_log_.get());
  return net_log_.get();
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

resource_coordinator::ResourceCoordinatorParts*
BrowserProcessImpl::resource_coordinator_parts() {
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
