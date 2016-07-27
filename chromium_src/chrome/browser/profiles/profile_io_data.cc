// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_io_data.h"

#include <stddef.h>

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug/alias.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
// #include "chrome/browser/browser_process.h"
// #include "chrome/browser/chrome_notification_types.h"
// #include "chrome/browser/content_settings/cookie_settings_factory.h"
// #include "chrome/browser/content_settings/host_content_settings_map_factory.h"
// #include "chrome/browser/custom_handlers/protocol_handler_registry.h"
// #include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
// #include "chrome/browser/devtools/devtools_network_controller.h"
// #include "chrome/browser/devtools/devtools_network_transaction_factory.h"
// #include "chrome/browser/download/download_service.h"
// #include "chrome/browser/download/download_service_factory.h"
// #include "chrome/browser/io_thread.h"
// #include "chrome/browser/media/media_device_id_salt.h"
// #include "chrome/browser/net/chrome_http_user_agent_settings.h"
// #include "chrome/browser/net/chrome_network_delegate.h"
// #include "chrome/browser/net/chrome_url_request_context_getter.h"
// #include "chrome/browser/net/proxy_service_factory.h"
// #include "chrome/browser/net/resource_prefetch_predictor_observer.h"
// #include "chrome/browser/policy/cloud/policy_header_service_factory.h"
// #include "chrome/browser/policy/policy_helpers.h"
// #include "chrome/browser/predictors/resource_prefetch_predictor.h"
// #include "chrome/browser/predictors/resource_prefetch_predictor_factory.h"
// #include "chrome/browser/profiles/profile.h"
// #include "chrome/browser/profiles/profile_manager.h"
// #include "chrome/browser/ssl/chrome_expect_ct_reporter.h"
// #include "chrome/browser/ui/search/new_tab_page_interceptor_service.h"
// #include "chrome/browser/ui/search/new_tab_page_interceptor_service_factory.h"
// #include "chrome/common/chrome_paths.h"
// #include "chrome/common/chrome_switches.h"
// #include "chrome/common/features.h"
#include "chrome/common/pref_names.h"
// #include "chrome/common/url_constants.h"
// #include "components/about_handler/about_protocol_handler.h"
#include "components/content_settings/core/browser/content_settings_provider.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
// #include "components/cookie_config/cookie_store_util.h"
// #include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
// #include "components/dom_distiller/core/url_constants.h"
// #include "components/metrics/metrics_pref_names.h"
// #include "components/metrics/metrics_service.h"
// #include "components/net_log/chrome_net_log.h"
// #include "components/policy/core/browser/url_blacklist_manager.h"
// #include "components/policy/core/common/cloud/policy_header_io_helper.h"
// #include "components/policy/core/common/cloud/policy_header_service.h"
// #include "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#include "components/prefs/pref_service.h"
// #include "components/signin/core/common/signin_pref_names.h"
// #include "components/sync_driver/pref_names.h"
// #include "components/url_formatter/url_fixer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/url_constants.h"
#include "net/base/keygen_handler.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cookies/canonical_cookie.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/http/http_util.h"
#include "net/http/transport_security_persister.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_script_fetcher_impl.h"
#include "net/proxy/proxy_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/client_cert_store.h"
#include "net/url_request/certificate_report_sender.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_file_job.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory_impl.h"

#if defined(ENABLE_EXTENSIONS)
// #include "chrome/browser/extensions/extension_cookie_monster_delegate.h"
// #include "chrome/browser/extensions/extension_resource_protocols.h"
#include "extensions/browser/extension_protocols.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_throttle_manager.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/constants.h"
#endif

// #if defined(ENABLE_SUPERVISED_USERS)
// #include "chrome/browser/supervised_user/supervised_user_service.h"
// #include "chrome/browser/supervised_user/supervised_user_service_factory.h"
// #include "chrome/browser/supervised_user/supervised_user_url_filter.h"
// #endif

// #if defined(OS_ANDROID)
// #include "content/public/browser/android/content_protocol_handler.h"
// #endif  // defined(OS_ANDROID)

// #if defined(OS_CHROMEOS)
// #include "chrome/browser/chromeos/certificate_provider/certificate_provider.h"
// #include "chrome/browser/chromeos/certificate_provider/certificate_provider_service.h"
// #include "chrome/browser/chromeos/certificate_provider/certificate_provider_service_factory.h"
// #include "chrome/browser/chromeos/fileapi/external_file_protocol_handler.h"
// #include "chrome/browser/chromeos/login/startup_utils.h"
// #include "chrome/browser/chromeos/net/cert_verify_proc_chromeos.h"
// #include "chrome/browser/chromeos/net/client_cert_filter_chromeos.h"
// #include "chrome/browser/chromeos/net/client_cert_store_chromeos.h"
// #include "chrome/browser/chromeos/policy/policy_cert_service.h"
// #include "chrome/browser/chromeos/policy/policy_cert_service_factory.h"
// #include "chrome/browser/chromeos/policy/policy_cert_verifier.h"
// #include "chrome/browser/chromeos/profiles/profile_helper.h"
// #include "chrome/browser/chromeos/settings/cros_settings.h"
// #include "chrome/browser/net/nss_context.h"
// #include "chromeos/dbus/dbus_thread_manager.h"
// #include "chromeos/settings/cros_settings_names.h"
// #include "chromeos/tpm/tpm_token_info_getter.h"
// #include "components/user_manager/user.h"
// #include "components/user_manager/user_manager.h"
// #include "crypto/nss_util.h"
// #include "crypto/nss_util_internal.h"
// #include "net/cert/cert_verifier.h"
// #include "net/cert/multi_threaded_cert_verifier.h"
// #endif  // defined(OS_CHROMEOS)

// #if defined(USE_NSS_CERTS)
// #include "chrome/browser/ui/crypto_module_delegate_nss.h"
// #include "net/ssl/client_cert_store_nss.h"
// #endif

// #if defined(OS_WIN)
// #include "net/ssl/client_cert_store_win.h"
// #endif

// #if defined(OS_MACOSX)
// #include "net/ssl/client_cert_store_mac.h"
// #endif

// using content::BrowserContext;
// using content::BrowserThread;
// using content::ResourceContext;

// namespace {

// net::CertVerifier* g_cert_verifier_for_testing = nullptr;

// #if defined(DEBUG_DEVTOOLS)
// bool IsSupportedDevToolsURL(const GURL& url, base::FilePath* path) {
//   std::string bundled_path_prefix(chrome::kChromeUIDevToolsBundledPath);
//   bundled_path_prefix = "/" + bundled_path_prefix + "/";

//   if (!url.SchemeIs(content::kChromeDevToolsScheme) ||
//       url.host_piece() != chrome::kChromeUIDevToolsHost ||
//       !base::StartsWith(url.path_piece(), bundled_path_prefix,
//                         base::CompareCase::INSENSITIVE_ASCII)) {
//     return false;
//   }

//   if (!url.is_valid()) {
//     NOTREACHED();
//     return false;
//   }

//   // Remove Query and Ref from URL.
//   GURL stripped_url;
//   GURL::Replacements replacements;
//   replacements.ClearQuery();
//   replacements.ClearRef();
//   stripped_url = url.ReplaceComponents(replacements);

//   std::string relative_path;
//   const std::string& spec = stripped_url.possibly_invalid_spec();
//   const url::Parsed& parsed = stripped_url.parsed_for_possibly_invalid_spec();
//   int offset = parsed.CountCharactersBefore(url::Parsed::PATH, false);
//   if (offset < static_cast<int>(spec.size()))
//     relative_path.assign(spec.substr(offset + bundled_path_prefix.length()));

//   // Check that |relative_path| is not an absolute path (otherwise
//   // AppendASCII() will DCHECK).  The awkward use of StringType is because on
//   // some systems FilePath expects a std::string, but on others a std::wstring.
//   base::FilePath p(
//       base::FilePath::StringType(relative_path.begin(), relative_path.end()));
//   if (p.IsAbsolute())
//     return false;

//   base::FilePath inspector_dir;
//   if (!PathService::Get(chrome::DIR_INSPECTOR, &inspector_dir))
//     return false;

//   if (inspector_dir.empty())
//     return false;

//   *path = inspector_dir.AppendASCII(relative_path);
//   return true;
// }

// class DebugDevToolsInterceptor : public net::URLRequestInterceptor {
//  public:
//   // net::URLRequestInterceptor implementation.
//   net::URLRequestJob* MaybeInterceptRequest(
//       net::URLRequest* request,
//       net::NetworkDelegate* network_delegate) const override {
//     base::FilePath path;
//     if (IsSupportedDevToolsURL(request->url(), &path))
//       return new net::URLRequestFileJob(
//           request, network_delegate, path,
//           content::BrowserThread::GetBlockingPool()->
//               GetTaskRunnerWithShutdownBehavior(
//                   base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));

//     return NULL;
//   }
// };
// #endif  // defined(DEBUG_DEVTOOLS)

// #if defined(OS_CHROMEOS)
// // The following four functions are responsible for initializing NSS for each
// // profile on ChromeOS, which has a separate NSS database and TPM slot
// // per-profile.
// //
// // Initialization basically follows these steps:
// // 1) Get some info from user_manager::UserManager about the User for this
// // profile.
// // 2) Tell nss_util to initialize the software slot for this profile.
// // 3) Wait for the TPM module to be loaded by nss_util if it isn't already.
// // 4) Ask CryptohomeClient which TPM slot id corresponds to this profile.
// // 5) Tell nss_util to use that slot id on the TPM module.
// //
// // Some of these steps must happen on the UI thread, others must happen on the
// // IO thread:
// //               UI thread                              IO Thread
// //
// //  ProfileIOData::InitializeOnUIThread
// //                   |
// //  ProfileHelper::Get()->GetUserByProfile()
// //                   \---------------------------------------v
// //                                                 StartNSSInitOnIOThread
// //                                                           |
// //                                          crypto::InitializeNSSForChromeOSUser
// //                                                           |
// //                                                crypto::IsTPMTokenReady
// //                                                           |
// //                                          StartTPMSlotInitializationOnIOThread
// //                   v---------------------------------------/
// //     GetTPMInfoForUserOnUIThread
// //                   |
// // chromeos::TPMTokenInfoGetter::Start
// //                   |
// //     DidGetTPMInfoForUserOnUIThread
// //                   \---------------------------------------v
// //                                          crypto::InitializeTPMForChromeOSUser

// void DidGetTPMInfoForUserOnUIThread(
//     std::unique_ptr<chromeos::TPMTokenInfoGetter> getter,
//     const std::string& username_hash,
//     const chromeos::TPMTokenInfo& info) {
//   DCHECK_CURRENTLY_ON(BrowserThread::UI);
//   if (info.tpm_is_enabled && info.token_slot_id != -1) {
//     DVLOG(1) << "Got TPM slot for " << username_hash << ": "
//              << info.token_slot_id;
//     BrowserThread::PostTask(
//         BrowserThread::IO,
//         FROM_HERE,
//         base::Bind(&crypto::InitializeTPMForChromeOSUser,
//                    username_hash, info.token_slot_id));
//   } else {
//     NOTREACHED() << "TPMTokenInfoGetter reported invalid token.";
//   }
// }

// void GetTPMInfoForUserOnUIThread(const AccountId& account_id,
//                                  const std::string& username_hash) {
//   DCHECK_CURRENTLY_ON(BrowserThread::UI);
//   DVLOG(1) << "Getting TPM info from cryptohome for "
//            << " " << account_id.Serialize() << " " << username_hash;
//   std::unique_ptr<chromeos::TPMTokenInfoGetter> scoped_token_info_getter =
//       chromeos::TPMTokenInfoGetter::CreateForUserToken(
//           account_id, chromeos::DBusThreadManager::Get()->GetCryptohomeClient(),
//           base::ThreadTaskRunnerHandle::Get());
//   chromeos::TPMTokenInfoGetter* token_info_getter =
//       scoped_token_info_getter.get();

//   // Bind |token_info_getter| to the callback to ensure it does not go away
//   // before TPM token info is fetched.
//   // TODO(tbarzic, pneubeck): Handle this in a nicer way when this logic is
//   //     moved to a separate profile service.
//   token_info_getter->Start(
//       base::Bind(&DidGetTPMInfoForUserOnUIThread,
//                  base::Passed(&scoped_token_info_getter),
//                  username_hash));
// }

// void StartTPMSlotInitializationOnIOThread(const AccountId& account_id,
//                                           const std::string& username_hash) {
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);

//   BrowserThread::PostTask(
//       BrowserThread::UI, FROM_HERE,
//       base::Bind(&GetTPMInfoForUserOnUIThread, account_id, username_hash));
// }

// void StartNSSInitOnIOThread(const AccountId& account_id,
//                             const std::string& username_hash,
//                             const base::FilePath& path) {
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);
//   DVLOG(1) << "Starting NSS init for " << account_id.Serialize()
//            << "  hash:" << username_hash;

//   // Make sure NSS is initialized for the user.
//   crypto::InitializeNSSForChromeOSUser(username_hash, path);

//   // Check if it's OK to initialize TPM for the user before continuing. This
//   // may not be the case if the TPM slot initialization was previously
//   // requested for the same user.
//   if (!crypto::ShouldInitializeTPMForChromeOSUser(username_hash))
//     return;

//   crypto::WillInitializeTPMForChromeOSUser(username_hash);

//   if (crypto::IsTPMTokenEnabledForNSS()) {
//     if (crypto::IsTPMTokenReady(
//             base::Bind(&StartTPMSlotInitializationOnIOThread, account_id,
//                        username_hash))) {
//       StartTPMSlotInitializationOnIOThread(account_id, username_hash);
//     } else {
//       DVLOG(1) << "Waiting for tpm ready ...";
//     }
//   } else {
//     crypto::InitializePrivateSoftwareSlotForChromeOSUser(username_hash);
//   }
// }
// #endif  // defined(OS_CHROMEOS)

// #if defined(USE_NSS_CERTS)
// void InitializeAndPassKeygenHandler(
//     std::unique_ptr<net::KeygenHandler> keygen_handler,
//     const base::Callback<void(std::unique_ptr<net::KeygenHandler>)>& callback,
//     std::unique_ptr<ChromeNSSCryptoModuleDelegate> delegate) {
//   if (delegate)
//     keygen_handler->set_crypto_module_delegate(std::move(delegate));
//   callback.Run(std::move(keygen_handler));
// }
// #endif  // defined(USE_NSS_CERTS)

// // For safe shutdown, must be called before the ProfileIOData is destroyed.
// void NotifyContextGettersOfShutdownOnIO(
//     std::unique_ptr<ProfileIOData::ChromeURLRequestContextGetterVector>
//         getters) {
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);
//   ProfileIOData::ChromeURLRequestContextGetterVector::iterator iter;
//   for (auto& chrome_context_getter : *getters)
//     chrome_context_getter->NotifyContextShuttingDown();
// }

// }  // namespace

// void ProfileIOData::InitializeOnUIThread(Profile* profile) {
//   DCHECK_CURRENTLY_ON(BrowserThread::UI);
//   PrefService* pref_service = profile->GetPrefs();

//   std::unique_ptr<ProfileParams> params(new ProfileParams);
//   params->path = profile->GetPath();

//   params->io_thread = g_browser_process->io_thread();

//   params->cookie_settings = CookieSettingsFactory::GetForProfile(profile);
//   params->host_content_settings_map =
//       HostContentSettingsMapFactory::GetForProfile(profile);
//   params->ssl_config_service = profile->GetSSLConfigService();

// #if defined(ENABLE_EXTENSIONS)
//   params->extension_info_map =
//       extensions::ExtensionSystem::Get(profile)->info_map();
//   params->cookie_monster_delegate = new ExtensionCookieMonsterDelegate(profile);
// #endif

//   if (predictors::ResourcePrefetchPredictor* predictor =
//           predictors::ResourcePrefetchPredictorFactory::GetForProfile(
//               profile)) {
//     resource_prefetch_predictor_observer_.reset(
//         new chrome_browser_net::ResourcePrefetchPredictorObserver(predictor));
//   }

//   ProtocolHandlerRegistry* protocol_handler_registry =
//       ProtocolHandlerRegistryFactory::GetForBrowserContext(profile);
//   DCHECK(protocol_handler_registry);

//   // The profile instance is only available here in the InitializeOnUIThread
//   // method, so we create the url job factory here, then save it for
//   // later delivery to the job factory in Init().
//   params->protocol_handler_interceptor =
//       protocol_handler_registry->CreateJobInterceptorFactory();

//   NewTabPageInterceptorService* new_tab_interceptor_service =
//       NewTabPageInterceptorServiceFactory::GetForProfile(profile);
//   if (new_tab_interceptor_service) {
//     params->new_tab_page_interceptor =
//         new_tab_interceptor_service->CreateInterceptor();
//   }

//   params->proxy_config_service = ProxyServiceFactory::CreateProxyConfigService(
//       profile->GetProxyConfigTracker());
// #if defined(ENABLE_SUPERVISED_USERS)
//   SupervisedUserService* supervised_user_service =
//       SupervisedUserServiceFactory::GetForProfile(profile);
//   params->supervised_user_url_filter =
//       supervised_user_service->GetURLFilterForIOThread();
// #endif
// #if defined(OS_CHROMEOS)
//   user_manager::UserManager* user_manager = user_manager::UserManager::Get();
//   if (user_manager) {
//     const user_manager::User* user =
//         chromeos::ProfileHelper::Get()->GetUserByProfile(profile);
//     // No need to initialize NSS for users with empty username hash:
//     // Getters for a user's NSS slots always return NULL slot if the user's
//     // username hash is empty, even when the NSS is not initialized for the
//     // user.
//     if (user && !user->username_hash().empty()) {
//       params->username_hash = user->username_hash();
//       DCHECK(!params->username_hash.empty());
//       BrowserThread::PostTask(
//           BrowserThread::IO, FROM_HERE,
//           base::Bind(&StartNSSInitOnIOThread, user->GetAccountId(),
//                      user->username_hash(), profile->GetPath()));

//       // Use the device-wide system key slot only if the user is affiliated on
//       // the device.
//       params->use_system_key_slot = user->IsAffiliated();
//     }
//   }

//   chromeos::CertificateProviderService* cert_provider_service =
//       chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
//           profile);
//   if (cert_provider_service) {
//     params->certificate_provider =
//         cert_provider_service->CreateCertificateProvider();
//   }
// #endif

//   params->profile = profile;
//   profile_params_.reset(params.release());

//   ChromeNetworkDelegate::InitializePrefsOnUIThread(
//       &enable_referrers_,
//       &enable_do_not_track_,
//       &force_google_safesearch_,
//       &force_youtube_safety_mode_,
//       pref_service);

//   scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
//       BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO);

//   chrome_http_user_agent_settings_.reset(
//       new ChromeHttpUserAgentSettings(pref_service));

//   // These members are used only for sign in, which is not enabled
//   // in incognito mode.  So no need to initialize them.
//   if (!IsOffTheRecord()) {
//     google_services_user_account_id_.Init(
//         prefs::kGoogleServicesUserAccountId, pref_service);
//     google_services_user_account_id_.MoveToThread(io_task_runner);

//     sync_disabled_.Init(sync_driver::prefs::kSyncManaged, pref_service);
//     sync_disabled_.MoveToThread(io_task_runner);

//     signin_allowed_.Init(prefs::kSigninAllowed, pref_service);
//     signin_allowed_.MoveToThread(io_task_runner);
//   }

//   media_device_id_salt_ = new MediaDeviceIDSalt(pref_service, IsOffTheRecord());

//   network_prediction_options_.Init(prefs::kNetworkPredictionOptions,
//                                    pref_service);

//   network_prediction_options_.MoveToThread(io_task_runner);

// #if defined(OS_CHROMEOS)
//   std::unique_ptr<policy::PolicyCertVerifier> verifier =
//       policy::PolicyCertServiceFactory::CreateForProfile(profile);
//   policy_cert_verifier_ = verifier.get();
//   cert_verifier_ = std::move(verifier);
// #endif
//   // The URLBlacklistManager has to be created on the UI thread to register
//   // observers of |pref_service|, and it also has to clean up on
//   // ShutdownOnUIThread to release these observers on the right thread.
//   // Don't pass it in |profile_params_| to make sure it is correctly cleaned up,
//   // in particular when this ProfileIOData isn't |initialized_| during deletion.
//   policy::URLBlacklist::SegmentURLCallback callback =
//       static_cast<policy::URLBlacklist::SegmentURLCallback>(
//           url_formatter::SegmentURL);
//   base::SequencedWorkerPool* pool = BrowserThread::GetBlockingPool();
//   scoped_refptr<base::SequencedTaskRunner> background_task_runner =
//       pool->GetSequencedTaskRunner(pool->GetSequenceToken());
//   url_blacklist_manager_.reset(new policy::URLBlacklistManager(
//       pref_service, background_task_runner, io_task_runner, callback,
//       base::Bind(policy::OverrideBlacklistForURL)));

//   if (!IsOffTheRecord()) {
//     // Add policy headers for non-incognito requests.
//     policy::PolicyHeaderService* policy_header_service =
//         policy::PolicyHeaderServiceFactory::GetForBrowserContext(profile);
//     if (policy_header_service) {
//       policy_header_helper_ =
//           policy_header_service->CreatePolicyHeaderIOHelper(io_task_runner);
//     }
//   }

//   incognito_availibility_pref_.Init(
//       prefs::kIncognitoModeAvailability, pref_service);
//   incognito_availibility_pref_.MoveToThread(io_task_runner);

//   // We need to make sure that content initializes its own data structures that
//   // are associated with each ResourceContext because we might post this
//   // object to the IO thread after this function.
//   BrowserContext::EnsureResourceContextInitialized(profile);
// }

// ProfileIOData::MediaRequestContext::MediaRequestContext() {
// }

// void ProfileIOData::MediaRequestContext::SetHttpTransactionFactory(
//     std::unique_ptr<net::HttpTransactionFactory> http_factory) {
//   http_factory_ = std::move(http_factory);
//   set_http_transaction_factory(http_factory_.get());
// }

// ProfileIOData::MediaRequestContext::~MediaRequestContext() {
//   AssertNoURLRequests();
// }

// ProfileIOData::AppRequestContext::AppRequestContext() {
// }

// void ProfileIOData::AppRequestContext::SetCookieStore(
//     std::unique_ptr<net::CookieStore> cookie_store) {
//   cookie_store_ = std::move(cookie_store);
//   set_cookie_store(cookie_store_.get());
// }

// void ProfileIOData::AppRequestContext::SetChannelIDService(
//     std::unique_ptr<net::ChannelIDService> channel_id_service) {
//   channel_id_service_ = std::move(channel_id_service);
//   set_channel_id_service(channel_id_service_.get());
// }

// void ProfileIOData::AppRequestContext::SetHttpNetworkSession(
//     std::unique_ptr<net::HttpNetworkSession> http_network_session) {
//   http_network_session_ = std::move(http_network_session);
// }

// void ProfileIOData::AppRequestContext::SetHttpTransactionFactory(
//     std::unique_ptr<net::HttpTransactionFactory> http_factory) {
//   http_factory_ = std::move(http_factory);
//   set_http_transaction_factory(http_factory_.get());
// }

// void ProfileIOData::AppRequestContext::SetJobFactory(
//     std::unique_ptr<net::URLRequestJobFactory> job_factory) {
//   job_factory_ = std::move(job_factory);
//   set_job_factory(job_factory_.get());
// }

// ProfileIOData::AppRequestContext::~AppRequestContext() {
//   AssertNoURLRequests();
// }

// ProfileIOData::ProfileParams::ProfileParams()
//     : io_thread(NULL),
// #if defined(OS_CHROMEOS)
//       use_system_key_slot(false),
// #endif
//       profile(NULL) {
// }

// ProfileIOData::ProfileParams::~ProfileParams() {}

// ProfileIOData::ProfileIOData(Profile::ProfileType profile_type)
//     : initialized_(false),
// #if defined(OS_CHROMEOS)
//       policy_cert_verifier_(NULL),
//       use_system_key_slot_(false),
// #endif
//       resource_context_(new ResourceContext(this)),
//       profile_type_(profile_type) {
//   DCHECK_CURRENTLY_ON(BrowserThread::UI);
// }

// ProfileIOData::~ProfileIOData() {
//   if (BrowserThread::IsMessageLoopValid(BrowserThread::IO))
//     DCHECK_CURRENTLY_ON(BrowserThread::IO);

//   // Pull the contents of the request context maps onto the stack for sanity
//   // checking of values in a minidump. http://crbug.com/260425
//   size_t num_app_contexts = app_request_context_map_.size();
//   size_t num_media_contexts = isolated_media_request_context_map_.size();
//   size_t current_context = 0;
//   static const size_t kMaxCachedContexts = 20;
//   net::URLRequestContext* app_context_cache[kMaxCachedContexts] = {0};
//   void* app_context_vtable_cache[kMaxCachedContexts] = {0};
//   net::URLRequestContext* media_context_cache[kMaxCachedContexts] = {0};
//   void* media_context_vtable_cache[kMaxCachedContexts] = {0};
//   void* tmp_vtable = NULL;
//   base::debug::Alias(&num_app_contexts);
//   base::debug::Alias(&num_media_contexts);
//   base::debug::Alias(&current_context);
//   base::debug::Alias(app_context_cache);
//   base::debug::Alias(app_context_vtable_cache);
//   base::debug::Alias(media_context_cache);
//   base::debug::Alias(media_context_vtable_cache);
//   base::debug::Alias(&tmp_vtable);

//   current_context = 0;
//   for (URLRequestContextMap::const_iterator it =
//            app_request_context_map_.begin();
//        current_context < kMaxCachedContexts &&
//            it != app_request_context_map_.end();
//        ++it, ++current_context) {
//     app_context_cache[current_context] = it->second;
//     memcpy(&app_context_vtable_cache[current_context],
//            static_cast<void*>(it->second), sizeof(void*));
//   }

//   current_context = 0;
//   for (URLRequestContextMap::const_iterator it =
//            isolated_media_request_context_map_.begin();
//        current_context < kMaxCachedContexts &&
//            it != isolated_media_request_context_map_.end();
//        ++it, ++current_context) {
//     media_context_cache[current_context] = it->second;
//     memcpy(&media_context_vtable_cache[current_context],
//            static_cast<void*>(it->second), sizeof(void*));
//   }

//   // Destroy certificate_report_sender_ before main_request_context_,
//   // since the former has a reference to the latter.
//   if (transport_security_state_)
//     transport_security_state_->SetReportSender(nullptr);
//   certificate_report_sender_.reset();

//   if (transport_security_state_)
//     transport_security_state_->SetExpectCTReporter(nullptr);
//   expect_ct_reporter_.reset();

//   // TODO(ajwong): These AssertNoURLRequests() calls are unnecessary since they
//   // are already done in the URLRequestContext destructor.
//   if (main_request_context_)
//     main_request_context_->AssertNoURLRequests();
//   if (extensions_request_context_)
//     extensions_request_context_->AssertNoURLRequests();

//   current_context = 0;
//   for (URLRequestContextMap::iterator it = app_request_context_map_.begin();
//        it != app_request_context_map_.end(); ++it) {
//     if (current_context < kMaxCachedContexts) {
//       CHECK_EQ(app_context_cache[current_context], it->second);
//       memcpy(&tmp_vtable, static_cast<void*>(it->second), sizeof(void*));
//       CHECK_EQ(app_context_vtable_cache[current_context], tmp_vtable);
//     }
//     it->second->AssertNoURLRequests();
//     delete it->second;
//     current_context++;
//   }

//   current_context = 0;
//   for (URLRequestContextMap::iterator it =
//            isolated_media_request_context_map_.begin();
//        it != isolated_media_request_context_map_.end(); ++it) {
//     if (current_context < kMaxCachedContexts) {
//       CHECK_EQ(media_context_cache[current_context], it->second);
//       memcpy(&tmp_vtable, static_cast<void*>(it->second), sizeof(void*));
//       CHECK_EQ(media_context_vtable_cache[current_context], tmp_vtable);
//     }
//     it->second->AssertNoURLRequests();
//     delete it->second;
//     current_context++;
//   }
// }

// // static
// ProfileIOData* ProfileIOData::FromResourceContext(
//     content::ResourceContext* rc) {
//   return (static_cast<ResourceContext*>(rc))->io_data_;
// }

// static
bool ProfileIOData::IsHandledProtocol(const std::string& scheme) {
  DCHECK_EQ(scheme, base::ToLowerASCII(scheme));
  static const char* const kProtocolList[] = {
    url::kFileScheme,
    content::kChromeDevToolsScheme,
    // dom_distiller::kDomDistillerScheme,
#if defined(ENABLE_EXTENSIONS)
    extensions::kExtensionScheme,
    extensions::kExtensionResourceScheme,
#endif
    content::kChromeUIScheme,
    url::kDataScheme,
#if defined(OS_CHROMEOS)
    content::kExternalFileScheme,
#endif  // defined(OS_CHROMEOS)
    url::kAboutScheme,
#if !defined(DISABLE_FTP_SUPPORT)
    url::kFtpScheme,
#endif  // !defined(DISABLE_FTP_SUPPORT)
    url::kBlobScheme,
    url::kFileSystemScheme,
    // chrome::kChromeSearchScheme,
  };
  for (size_t i = 0; i < arraysize(kProtocolList); ++i) {
    if (scheme == kProtocolList[i])
      return true;
  }
  return net::URLRequest::IsHandledProtocol(scheme);
}

// static
bool ProfileIOData::IsHandledURL(const GURL& url) {
  if (!url.is_valid()) {
    // We handle error cases.
    return true;
  }

  return IsHandledProtocol(url.scheme());
}

// static
void ProfileIOData::InstallProtocolHandlers(
    net::URLRequestJobFactoryImpl* job_factory,
    content::ProtocolHandlerMap* protocol_handlers) {
  for (content::ProtocolHandlerMap::iterator it =
           protocol_handlers->begin();
       it != protocol_handlers->end();
       ++it) {
    bool set_protocol = job_factory->SetProtocolHandler(
        it->first, base::WrapUnique(it->second.release()));
    DCHECK(set_protocol);
  }
  protocol_handlers->clear();
}

// // static
// void ProfileIOData::SetCertVerifierForTesting(
//     net::CertVerifier* cert_verifier) {
//   g_cert_verifier_for_testing = cert_verifier;
// }

// content::ResourceContext* ProfileIOData::GetResourceContext() const {
//   return resource_context_.get();
// }

// net::URLRequestContext* ProfileIOData::GetMainRequestContext() const {
//   DCHECK(initialized_);
//   return main_request_context_.get();
// }

// net::URLRequestContext* ProfileIOData::GetMediaRequestContext() const {
//   DCHECK(initialized_);
//   net::URLRequestContext* context = AcquireMediaRequestContext();
//   DCHECK(context);
//   return context;
// }

// net::URLRequestContext* ProfileIOData::GetExtensionsRequestContext() const {
//   DCHECK(initialized_);
//   return extensions_request_context_.get();
// }

// net::URLRequestContext* ProfileIOData::GetIsolatedAppRequestContext(
//     net::URLRequestContext* main_context,
//     const StoragePartitionDescriptor& partition_descriptor,
//     std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//         protocol_handler_interceptor,
//     content::ProtocolHandlerMap* protocol_handlers,
//     content::URLRequestInterceptorScopedVector request_interceptors) const {
//   DCHECK(initialized_);
//   net::URLRequestContext* context = NULL;
//   if (ContainsKey(app_request_context_map_, partition_descriptor)) {
//     context = app_request_context_map_[partition_descriptor];
//   } else {
//     context = AcquireIsolatedAppRequestContext(
//         main_context, partition_descriptor,
//         std::move(protocol_handler_interceptor), protocol_handlers,
//         std::move(request_interceptors));
//     app_request_context_map_[partition_descriptor] = context;
//   }
//   DCHECK(context);
//   return context;
// }

// net::URLRequestContext* ProfileIOData::GetIsolatedMediaRequestContext(
//     net::URLRequestContext* app_context,
//     const StoragePartitionDescriptor& partition_descriptor) const {
//   DCHECK(initialized_);
//   net::URLRequestContext* context = NULL;
//   if (ContainsKey(isolated_media_request_context_map_, partition_descriptor)) {
//     context = isolated_media_request_context_map_[partition_descriptor];
//   } else {
//     context = AcquireIsolatedMediaRequestContext(app_context,
//                                                  partition_descriptor);
//     isolated_media_request_context_map_[partition_descriptor] = context;
//   }
//   DCHECK(context);
//   return context;
// }

// extensions::InfoMap* ProfileIOData::GetExtensionInfoMap() const {
//   DCHECK(initialized_) << "ExtensionSystem not initialized";
// #if defined(ENABLE_EXTENSIONS)
//   return extension_info_map_.get();
// #else
//   return nullptr;
// #endif
// }

// extensions::ExtensionThrottleManager*
// ProfileIOData::GetExtensionThrottleManager() const {
//   DCHECK(initialized_) << "ExtensionSystem not initialized";
// #if defined(ENABLE_EXTENSIONS)
//   return extension_throttle_manager_.get();
// #else
//   return nullptr;
// #endif
// }

// content_settings::CookieSettings* ProfileIOData::GetCookieSettings() const {
//   // Allow either Init() or SetCookieSettingsForTesting() to initialize.
//   DCHECK(initialized_ || cookie_settings_.get());
//   return cookie_settings_.get();
// }

// HostContentSettingsMap* ProfileIOData::GetHostContentSettingsMap() const {
//   DCHECK(initialized_);
//   return host_content_settings_map_.get();
// }

// ResourceContext::SaltCallback ProfileIOData::GetMediaDeviceIDSalt() const {
//   return base::Bind(&MediaDeviceIDSalt::GetSalt, media_device_id_salt_);
// }

// bool ProfileIOData::IsOffTheRecord() const {
//   return profile_type() == Profile::INCOGNITO_PROFILE
//       || profile_type() == Profile::GUEST_PROFILE;
// }

// void ProfileIOData::InitializeMetricsEnabledStateOnUIThread() {
//   DCHECK_CURRENTLY_ON(BrowserThread::UI);
// #if BUILDFLAG(ANDROID_JAVA_UI)
//   // TODO(dwkang): rename or unify the pref for UMA once we have conclusion
//   // in crbugs.com/246495.
//   // Android has it's own preferences for metrics / crash uploading.
//   enable_metrics_.Init(prefs::kCrashReportingEnabled,
//                        g_browser_process->local_state());
//   enable_metrics_.MoveToThread(
//       BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO));
// #else
//   // Prep the PrefMember and send it to the IO thread, since this value will be
//   // read from there.
//   enable_metrics_.Init(metrics::prefs::kMetricsReportingEnabled,
//                        g_browser_process->local_state());
//   enable_metrics_.MoveToThread(
//       BrowserThread::GetMessageLoopProxyForThread(BrowserThread::IO));
// #endif  // BUILDFLAG(ANDROID_JAVA_UI)
// }

// bool ProfileIOData::GetMetricsEnabledStateOnIOThread() const {
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);
//   return enable_metrics_.GetValue();
// }

// bool ProfileIOData::IsDataReductionProxyEnabled() const {
//   return data_reduction_proxy_io_data() &&
//       data_reduction_proxy_io_data()->IsEnabled();
// }

// void ProfileIOData::set_data_reduction_proxy_io_data(
//     std::unique_ptr<data_reduction_proxy::DataReductionProxyIOData>
//         data_reduction_proxy_io_data) const {
//   data_reduction_proxy_io_data_ = std::move(data_reduction_proxy_io_data);
// }

// base::WeakPtr<net::HttpServerProperties>
// ProfileIOData::http_server_properties() const {
//   return http_server_properties_->GetWeakPtr();
// }

// void ProfileIOData::set_http_server_properties(
//     std::unique_ptr<net::HttpServerProperties> http_server_properties) const {
//   http_server_properties_ = std::move(http_server_properties);
// }

// ProfileIOData::ResourceContext::ResourceContext(ProfileIOData* io_data)
//     : io_data_(io_data),
//       host_resolver_(NULL),
//       request_context_(NULL) {
//   DCHECK(io_data);
// }

// ProfileIOData::ResourceContext::~ResourceContext() {}

// net::HostResolver* ProfileIOData::ResourceContext::GetHostResolver()  {
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);
//   DCHECK(io_data_->initialized_);
//   return host_resolver_;
// }

// net::URLRequestContext* ProfileIOData::ResourceContext::GetRequestContext()  {
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);
//   DCHECK(io_data_->initialized_);
//   return request_context_;
// }

// std::unique_ptr<net::ClientCertStore>
// ProfileIOData::ResourceContext::CreateClientCertStore() {
//   if (!io_data_->client_cert_store_factory_.is_null())
//     return io_data_->client_cert_store_factory_.Run();
// #if defined(OS_CHROMEOS)
//   return std::unique_ptr<net::ClientCertStore>(
//       new chromeos::ClientCertStoreChromeOS(
//           io_data_->certificate_provider_
//               ? io_data_->certificate_provider_->Copy()
//               : nullptr,
//           base::WrapUnique(new chromeos::ClientCertFilterChromeOS(
//               io_data_->use_system_key_slot(), io_data_->username_hash())),
//           base::Bind(&CreateCryptoModuleBlockingPasswordDelegate,
//                      chrome::kCryptoModulePasswordClientAuth)));
// #elif defined(USE_NSS_CERTS)
//   return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreNSS(
//       base::Bind(&CreateCryptoModuleBlockingPasswordDelegate,
//                  chrome::kCryptoModulePasswordClientAuth)));
// #elif defined(OS_WIN)
//   return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreWin());
// #elif defined(OS_MACOSX)
//   return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreMac());
// #elif defined(OS_ANDROID)
//   // Android does not use the ClientCertStore infrastructure. On Android client
//   // cert matching is done by the OS as part of the call to show the cert
//   // selection dialog.
//   return nullptr;
// #else
// #error Unknown platform.
// #endif
// }

// void ProfileIOData::ResourceContext::CreateKeygenHandler(
//     uint32_t key_size_in_bits,
//     const std::string& challenge_string,
//     const GURL& url,
//     const base::Callback<void(std::unique_ptr<net::KeygenHandler>)>& callback) {
//   DCHECK(!callback.is_null());
// #if defined(USE_NSS_CERTS)
//   std::unique_ptr<net::KeygenHandler> keygen_handler(
//       new net::KeygenHandler(key_size_in_bits, challenge_string, url));

//   base::Callback<void(std::unique_ptr<ChromeNSSCryptoModuleDelegate>)>
//       got_delegate_callback =
//           base::Bind(&InitializeAndPassKeygenHandler,
//                      base::Passed(&keygen_handler), callback);

//   ChromeNSSCryptoModuleDelegate::CreateForResourceContext(
//       chrome::kCryptoModulePasswordKeygen,
//       net::HostPortPair::FromURL(url),
//       this,
//       got_delegate_callback);
// #else
//   callback.Run(base::WrapUnique(
//       new net::KeygenHandler(key_size_in_bits, challenge_string, url)));
// #endif
// }

// ResourceContext::SaltCallback
// ProfileIOData::ResourceContext::GetMediaDeviceIDSalt() {
//   return io_data_->GetMediaDeviceIDSalt();
// }

// void ProfileIOData::Init(
//     content::ProtocolHandlerMap* protocol_handlers,
//     content::URLRequestInterceptorScopedVector request_interceptors) const {
//   // The basic logic is implemented here. The specific initialization
//   // is done in InitializeInternal(), implemented by subtypes. Static helper
//   // functions have been provided to assist in common operations.
//   DCHECK_CURRENTLY_ON(BrowserThread::IO);
//   DCHECK(!initialized_);
//   DCHECK(profile_params_.get());

//   IOThread* const io_thread = profile_params_->io_thread;
//   IOThread::Globals* const io_thread_globals = io_thread->globals();
//   const base::CommandLine& command_line =
//       *base::CommandLine::ForCurrentProcess();

//   // Create the common request contexts.
//   main_request_context_.reset(new net::URLRequestContext());
//   extensions_request_context_.reset(new net::URLRequestContext());

//   main_request_context_->set_enable_brotli(io_thread_globals->enable_brotli);

//   std::unique_ptr<ChromeNetworkDelegate> network_delegate(
//       new ChromeNetworkDelegate(
// #if defined(ENABLE_EXTENSIONS)
//           io_thread_globals->extension_event_router_forwarder.get(),
// #else
//           NULL,
// #endif
//           &enable_referrers_, io_thread->GetMetricsDataUseForwarder()));
// #if defined(ENABLE_EXTENSIONS)
//   network_delegate->set_extension_info_map(
//       profile_params_->extension_info_map.get());
//   if (!command_line.HasSwitch(switches::kDisableExtensionsHttpThrottling)) {
//     extension_throttle_manager_.reset(
//         new extensions::ExtensionThrottleManager());
//   }
// #endif

//   network_delegate->set_url_blacklist_manager(url_blacklist_manager_.get());
//   network_delegate->set_profile(profile_params_->profile);
//   network_delegate->set_profile_path(profile_params_->path);
//   network_delegate->set_cookie_settings(profile_params_->cookie_settings.get());
//   network_delegate->set_enable_do_not_track(&enable_do_not_track_);
//   network_delegate->set_force_google_safe_search(&force_google_safesearch_);
//   network_delegate->set_force_youtube_safety_mode(&force_youtube_safety_mode_);
//   network_delegate->set_data_use_aggregator(
//       io_thread_globals->data_use_aggregator.get(), IsOffTheRecord());

//   // NOTE: Proxy service uses the default io thread network delegate, not the
//   // delegate just created.
//   proxy_service_ = ProxyServiceFactory::CreateProxyService(
//       io_thread->net_log(),
//       io_thread_globals->proxy_script_fetcher_context.get(),
//       io_thread_globals->system_network_delegate.get(),
//       std::move(profile_params_->proxy_config_service), command_line,
//       io_thread->WpadQuickCheckEnabled(),
//       io_thread->PacHttpsUrlStrippingEnabled());
//   transport_security_state_.reset(new net::TransportSecurityState());
//   base::SequencedWorkerPool* pool = BrowserThread::GetBlockingPool();
//   transport_security_persister_.reset(
//       new net::TransportSecurityPersister(
//           transport_security_state_.get(),
//           profile_params_->path,
//           pool->GetSequencedTaskRunnerWithShutdownBehavior(
//               pool->GetSequenceToken(),
//               base::SequencedWorkerPool::BLOCK_SHUTDOWN),
//           IsOffTheRecord()));

//   certificate_report_sender_.reset(new net::CertificateReportSender(
//       main_request_context_.get(),
//       net::CertificateReportSender::DO_NOT_SEND_COOKIES));
//   transport_security_state_->SetReportSender(certificate_report_sender_.get());

//   expect_ct_reporter_.reset(
//       new ChromeExpectCTReporter(main_request_context_.get()));
//   transport_security_state_->SetExpectCTReporter(expect_ct_reporter_.get());

//   // Take ownership over these parameters.
//   cookie_settings_ = profile_params_->cookie_settings;
//   host_content_settings_map_ = profile_params_->host_content_settings_map;
// #if defined(ENABLE_EXTENSIONS)
//   extension_info_map_ = profile_params_->extension_info_map;
// #endif

//   resource_context_->host_resolver_ = io_thread_globals->host_resolver.get();
//   resource_context_->request_context_ = main_request_context_.get();

//   if (profile_params_->resource_prefetch_predictor_observer_) {
//     resource_prefetch_predictor_observer_.reset(
//         profile_params_->resource_prefetch_predictor_observer_.release());
//   }

// #if defined(ENABLE_SUPERVISED_USERS)
//   supervised_user_url_filter_ = profile_params_->supervised_user_url_filter;
// #endif

// #if defined(OS_CHROMEOS)
//   username_hash_ = profile_params_->username_hash;
//   use_system_key_slot_ = profile_params_->use_system_key_slot;
//   if (use_system_key_slot_)
//     EnableNSSSystemKeySlotForResourceContext(resource_context_.get());

//   certificate_provider_ = std::move(profile_params_->certificate_provider);
// #endif

//   if (g_cert_verifier_for_testing) {
//     main_request_context_->set_cert_verifier(g_cert_verifier_for_testing);
//   } else {
// #if defined(OS_CHROMEOS)
//     crypto::ScopedPK11Slot public_slot =
//         crypto::GetPublicSlotForChromeOSUser(username_hash_);
//     // The private slot won't be ready by this point. It shouldn't be necessary
//     // for cert trust purposes anyway.
//     scoped_refptr<net::CertVerifyProc> verify_proc(
//         new chromeos::CertVerifyProcChromeOS(std::move(public_slot)));
//     if (policy_cert_verifier_) {
//       DCHECK_EQ(policy_cert_verifier_, cert_verifier_.get());
//       policy_cert_verifier_->InitializeOnIOThread(verify_proc);
//     } else {
//       cert_verifier_.reset(
//           new net::MultiThreadedCertVerifier(verify_proc.get()));
//     }
//     main_request_context_->set_cert_verifier(cert_verifier_.get());
// #else
//     main_request_context_->set_cert_verifier(
//         io_thread_globals->cert_verifier.get());
// #endif
//   }

//   // Install the New Tab Page Interceptor.
//   if (profile_params_->new_tab_page_interceptor.get()) {
//     request_interceptors.push_back(
//         profile_params_->new_tab_page_interceptor.release());
//   }

//   std::unique_ptr<net::MultiLogCTVerifier> ct_verifier(
//       new net::MultiLogCTVerifier());
//   ct_verifier->AddLogs(io_thread_globals->ct_logs);
//   main_request_context_->set_cert_transparency_verifier(ct_verifier.get());
//   cert_transparency_verifier_ = std::move(ct_verifier);

//   InitializeInternal(std::move(network_delegate), profile_params_.get(),
//                      protocol_handlers, std::move(request_interceptors));

//   profile_params_.reset();
//   initialized_ = true;
// }

// void ProfileIOData::ApplyProfileParamsToContext(
//     net::URLRequestContext* context) const {
//   context->set_http_user_agent_settings(
//       chrome_http_user_agent_settings_.get());
//   context->set_ssl_config_service(profile_params_->ssl_config_service.get());
// }

// std::unique_ptr<net::URLRequestJobFactory>
// ProfileIOData::SetUpJobFactoryDefaults(
//     std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory,
//     content::URLRequestInterceptorScopedVector request_interceptors,
//     std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//         protocol_handler_interceptor,
//     net::NetworkDelegate* network_delegate,
//     net::FtpTransactionFactory* ftp_transaction_factory) const {
//   // NOTE(willchan): Keep these protocol handlers in sync with
//   // ProfileIOData::IsHandledProtocol().
//   bool set_protocol = job_factory->SetProtocolHandler(
//       url::kFileScheme,
//       base::WrapUnique(new net::FileProtocolHandler(
//           content::BrowserThread::GetBlockingPool()
//               ->GetTaskRunnerWithShutdownBehavior(
//                   base::SequencedWorkerPool::SKIP_ON_SHUTDOWN))));
//   DCHECK(set_protocol);

// #if defined(ENABLE_EXTENSIONS)
//   DCHECK(extension_info_map_.get());
//   // Check only for incognito (and not Chrome OS guest mode GUEST_PROFILE).
//   bool is_incognito = profile_type() == Profile::INCOGNITO_PROFILE;
//   set_protocol = job_factory->SetProtocolHandler(
//       extensions::kExtensionScheme,
//       extensions::CreateExtensionProtocolHandler(is_incognito,
//                                                  extension_info_map_.get()));
//   DCHECK(set_protocol);
//   set_protocol = job_factory->SetProtocolHandler(
//       extensions::kExtensionResourceScheme,
//       CreateExtensionResourceProtocolHandler());
//   DCHECK(set_protocol);
// #endif
//   set_protocol = job_factory->SetProtocolHandler(
//       url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler()));
//   DCHECK(set_protocol);
// #if defined(OS_CHROMEOS)
//   if (profile_params_) {
//     set_protocol = job_factory->SetProtocolHandler(
//         content::kExternalFileScheme,
//         base::WrapUnique(new chromeos::ExternalFileProtocolHandler(
//             profile_params_->profile)));
//     DCHECK(set_protocol);
//   }
// #endif  // defined(OS_CHROMEOS)
// #if defined(OS_ANDROID)
//   set_protocol = job_factory->SetProtocolHandler(
//       url::kContentScheme,
//       content::ContentProtocolHandler::Create(
//           content::BrowserThread::GetBlockingPool()
//               ->GetTaskRunnerWithShutdownBehavior(
//                   base::SequencedWorkerPool::SKIP_ON_SHUTDOWN)));
// #endif

//   job_factory->SetProtocolHandler(
//       url::kAboutScheme,
//       base::WrapUnique(new about_handler::AboutProtocolHandler()));
// #if !defined(DISABLE_FTP_SUPPORT)
//   DCHECK(ftp_transaction_factory);
//   job_factory->SetProtocolHandler(
//       url::kFtpScheme,
//       base::WrapUnique(new net::FtpProtocolHandler(ftp_transaction_factory)));
// #endif  // !defined(DISABLE_FTP_SUPPORT)

// #if defined(DEBUG_DEVTOOLS)
//   request_interceptors.push_back(new DebugDevToolsInterceptor);
// #endif

//   // Set up interceptors in the reverse order.
//   std::unique_ptr<net::URLRequestJobFactory> top_job_factory =
//       std::move(job_factory);
//   for (content::URLRequestInterceptorScopedVector::reverse_iterator i =
//            request_interceptors.rbegin();
//        i != request_interceptors.rend();
//        ++i) {
//     top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
//         std::move(top_job_factory), base::WrapUnique(*i)));
//   }
//   request_interceptors.weak_clear();

//   if (protocol_handler_interceptor) {
//     protocol_handler_interceptor->Chain(std::move(top_job_factory));
//     return std::move(protocol_handler_interceptor);
//   } else {
//     return top_job_factory;
//   }
// }

// void ProfileIOData::ShutdownOnUIThread(
//     std::unique_ptr<ChromeURLRequestContextGetterVector> context_getters) {
//   DCHECK_CURRENTLY_ON(BrowserThread::UI);

//   google_services_user_account_id_.Destroy();
//   enable_referrers_.Destroy();
//   enable_do_not_track_.Destroy();
//   force_google_safesearch_.Destroy();
//   force_youtube_safety_mode_.Destroy();
//   enable_metrics_.Destroy();
//   safe_browsing_enabled_.Destroy();
//   sync_disabled_.Destroy();
//   signin_allowed_.Destroy();
//   network_prediction_options_.Destroy();
//   if (media_device_id_salt_.get())
//     media_device_id_salt_->ShutdownOnUIThread();
//   session_startup_pref_.Destroy();
//   if (url_blacklist_manager_)
//     url_blacklist_manager_->ShutdownOnUIThread();
//   if (chrome_http_user_agent_settings_)
//     chrome_http_user_agent_settings_->CleanupOnUIThread();
//   incognito_availibility_pref_.Destroy();

//   if (!context_getters->empty()) {
//     if (BrowserThread::IsMessageLoopValid(BrowserThread::IO)) {
//       BrowserThread::PostTask(
//           BrowserThread::IO, FROM_HERE,
//           base::Bind(&NotifyContextGettersOfShutdownOnIO,
//               base::Passed(&context_getters)));
//     }
//   }

//   bool posted = BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, this);
//   if (!posted)
//     delete this;
// }

// void ProfileIOData::set_channel_id_service(
//     net::ChannelIDService* channel_id_service) const {
//   channel_id_service_.reset(channel_id_service);
// }

// void ProfileIOData::DestroyResourceContext() {
//   resource_context_.reset();
// }

// std::unique_ptr<net::HttpNetworkSession>
// ProfileIOData::CreateHttpNetworkSession(
//     const ProfileParams& profile_params) const {
//   net::URLRequestContext* context = main_request_context();

//   IOThread* const io_thread = profile_params.io_thread;

//   net::HttpNetworkSession::Params params(io_thread->NetworkSessionParams());
//   net::URLRequestContextBuilder::SetHttpNetworkSessionComponents(context,
//                                                                  &params);
//   if (!IsOffTheRecord() && io_thread->globals()->network_quality_estimator) {
//     params.socket_performance_watcher_factory =
//         io_thread->globals()
//             ->network_quality_estimator->GetSocketPerformanceWatcherFactory();
//   }
//   if (data_reduction_proxy_io_data_.get())
//     params.proxy_delegate = data_reduction_proxy_io_data_->proxy_delegate();

//   return std::unique_ptr<net::HttpNetworkSession>(
//       new net::HttpNetworkSession(params));
// }

// std::unique_ptr<net::HttpCache> ProfileIOData::CreateMainHttpFactory(
//     net::HttpNetworkSession* session,
//     std::unique_ptr<net::HttpCache::BackendFactory> main_backend) const {
//   return base::WrapUnique(new net::HttpCache(
//       base::WrapUnique(new DevToolsNetworkTransactionFactory(
//           network_controller_handle_.GetController(), session)),
//       std::move(main_backend), true /* set_up_quic_server_info */));
// }

// std::unique_ptr<net::HttpCache> ProfileIOData::CreateHttpFactory(
//     net::HttpNetworkSession* shared_session,
//     std::unique_ptr<net::HttpCache::BackendFactory> backend) const {
//   return base::WrapUnique(new net::HttpCache(
//       base::WrapUnique(new DevToolsNetworkTransactionFactory(
//           network_controller_handle_.GetController(), shared_session)),
//       std::move(backend), true /* set_up_quic_server_info */));
// }

// void ProfileIOData::SetCookieSettingsForTesting(
//     content_settings::CookieSettings* cookie_settings) {
//   DCHECK(!cookie_settings_.get());
//   cookie_settings_ = cookie_settings;
// }
