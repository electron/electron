// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_
#define CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
// #include "chrome/browser/devtools/devtools_network_controller_handle.h"
// #include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile.h"
// #include "chrome/browser/profiles/storage_partition_descriptor.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"
#include "net/cert/ct_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_session.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory.h"

// class ChromeHttpUserAgentSettings;
// class ChromeNetworkDelegate;
// class ChromeURLRequestContextGetter;
// class ChromeExpectCTReporter;
// class HostContentSettingsMap;
// class MediaDeviceIDSalt;
// class ProtocolHandlerRegistry;
// class SupervisedUserURLFilter;

// namespace chromeos {
// class CertificateProvider;
// }

// namespace chrome_browser_net {
// class ResourcePrefetchPredictorObserver;
// }

namespace content_settings {
class CookieSettings;
}

// namespace data_reduction_proxy {
// class DataReductionProxyIOData;
// }

namespace extensions {
class ExtensionThrottleManager;
class InfoMap;
}

namespace net {
class CertificateReportSender;
class CertVerifier;
class ChannelIDService;
class CookieStore;
class FtpTransactionFactory;
class HttpServerProperties;
class HttpTransactionFactory;
class ProxyConfigService;
class ProxyService;
class SSLConfigService;
class TransportSecurityPersister;
class TransportSecurityState;
class URLRequestJobFactoryImpl;
}  // namespace net

namespace policy {
class PolicyCertVerifier;
class PolicyHeaderIOHelper;
class URLBlacklistManager;
}  // namespace policy

// Conceptually speaking, the ProfileIOData represents data that lives on the IO
// thread that is owned by a Profile, such as, but not limited to, network
// objects like CookieMonster, HttpTransactionFactory, etc.  Profile owns
// ProfileIOData, but will make sure to delete it on the IO thread (except
// possibly in unit tests where there is no IO thread).
class ProfileIOData {
 public:
  // typedef std::vector<scoped_refptr<ChromeURLRequestContextGetter>>
  //     ChromeURLRequestContextGetterVector;

  // virtual ~ProfileIOData();

  // static ProfileIOData* FromResourceContext(content::ResourceContext* rc);

  // Returns true if |scheme| is handled in Chrome, or by default handlers in
  // net::URLRequest.
  static bool IsHandledProtocol(const std::string& scheme);

  // Returns true if |url| is handled in Chrome, or by default handlers in
  // net::URLRequest.
  static bool IsHandledURL(const GURL& url);

  // Utility to install additional WebUI handlers into the |job_factory|.
  // Ownership of the handlers is transfered from |protocol_handlers|
  // to the |job_factory|.
  static void InstallProtocolHandlers(
      net::URLRequestJobFactoryImpl* job_factory,
      content::ProtocolHandlerMap* protocol_handlers);

//   // Sets a global CertVerifier to use when initializing all profiles.
//   static void SetCertVerifierForTesting(net::CertVerifier* cert_verifier);

//   // Called by Profile.
//   content::ResourceContext* GetResourceContext() const;

//   // Initializes the ProfileIOData object and primes the RequestContext
//   // generation. Must be called prior to any of the Get*() methods other than
//   // GetResouceContext or GetMetricsEnabledStateOnIOThread.
//   void Init(
//       content::ProtocolHandlerMap* protocol_handlers,
//       content::URLRequestInterceptorScopedVector request_interceptors) const;

//   net::URLRequestContext* GetMainRequestContext() const;
//   net::URLRequestContext* GetMediaRequestContext() const;
//   net::URLRequestContext* GetExtensionsRequestContext() const;
//   net::URLRequestContext* GetIsolatedAppRequestContext(
//       net::URLRequestContext* main_context,
//       const StoragePartitionDescriptor& partition_descriptor,
//       std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//           protocol_handler_interceptor,
//       content::ProtocolHandlerMap* protocol_handlers,
//       content::URLRequestInterceptorScopedVector request_interceptors) const;
//   net::URLRequestContext* GetIsolatedMediaRequestContext(
//       net::URLRequestContext* app_context,
//       const StoragePartitionDescriptor& partition_descriptor) const;

//   // These are useful when the Chrome layer is called from the content layer
//   // with a content::ResourceContext, and they want access to Chrome data for
//   // that profile.
//   extensions::InfoMap* GetExtensionInfoMap() const;
//   extensions::ExtensionThrottleManager* GetExtensionThrottleManager() const;
//   content_settings::CookieSettings* GetCookieSettings() const;
//   HostContentSettingsMap* GetHostContentSettingsMap() const;

//   IntegerPrefMember* session_startup_pref() const {
//     return &session_startup_pref_;
//   }

//   StringPrefMember* google_services_account_id() const {
//     return &google_services_user_account_id_;
//   }

//   net::URLRequestContext* extensions_request_context() const {
//     return extensions_request_context_.get();
//   }

//   BooleanPrefMember* safe_browsing_enabled() const {
//     return &safe_browsing_enabled_;
//   }

//   BooleanPrefMember* sync_disabled() const {
//     return &sync_disabled_;
//   }

//   BooleanPrefMember* signin_allowed() const {
//     return &signin_allowed_;
//   }

//   IntegerPrefMember* network_prediction_options() const {
//     return &network_prediction_options_;
//   }

//   content::ResourceContext::SaltCallback GetMediaDeviceIDSalt() const;

//   DevToolsNetworkControllerHandle* network_controller_handle() const {
//     return &network_controller_handle_;
//   }

//   net::TransportSecurityState* transport_security_state() const {
//     return transport_security_state_.get();
//   }

// #if defined(OS_CHROMEOS)
//   std::string username_hash() const {
//     return username_hash_;
//   }

//   bool use_system_key_slot() const { return use_system_key_slot_; }
// #endif

//   Profile::ProfileType profile_type() const {
//     return profile_type_;
//   }

//   bool IsOffTheRecord() const;

//   IntegerPrefMember* incognito_availibility() const {
//     return &incognito_availibility_pref_;
//   }

//   chrome_browser_net::ResourcePrefetchPredictorObserver*
//       resource_prefetch_predictor_observer() const {
//     return resource_prefetch_predictor_observer_.get();
//   }

//   policy::PolicyHeaderIOHelper* policy_header_helper() const {
//     return policy_header_helper_.get();
//   }

// #if defined(ENABLE_SUPERVISED_USERS)
//   const SupervisedUserURLFilter* supervised_user_url_filter() const {
//     return supervised_user_url_filter_.get();
//   }
// #endif

//   // Initialize the member needed to track the metrics enabled state. This is
//   // only to be called on the UI thread.
//   void InitializeMetricsEnabledStateOnUIThread();

//   // Returns whether or not metrics reporting is enabled in the browser instance
//   // on which this profile resides. This is safe for use from the IO thread, and
//   // should only be called from there.
//   bool GetMetricsEnabledStateOnIOThread() const;

//   void set_client_cert_store_factory_for_testing(
//       const base::Callback<std::unique_ptr<net::ClientCertStore>()>& factory) {
//     client_cert_store_factory_ = factory;
//   }

//   bool IsDataReductionProxyEnabled() const;

//   data_reduction_proxy::DataReductionProxyIOData*
//   data_reduction_proxy_io_data() const {
//     return data_reduction_proxy_io_data_.get();
//   }

 protected:
//   // A URLRequestContext for media that owns its HTTP factory, to ensure
//   // it is deleted.
//   class MediaRequestContext : public net::URLRequestContext {
//    public:
//     MediaRequestContext();

//     void SetHttpTransactionFactory(
//         std::unique_ptr<net::HttpTransactionFactory> http_factory);

//    private:
//     ~MediaRequestContext() override;

//     std::unique_ptr<net::HttpTransactionFactory> http_factory_;
//   };

//   // A URLRequestContext for apps that owns its cookie store and HTTP factory,
//   // to ensure they are deleted.
//   class AppRequestContext : public net::URLRequestContext {
//    public:
//     AppRequestContext();

//     void SetCookieStore(std::unique_ptr<net::CookieStore> cookie_store);
//     void SetChannelIDService(
//         std::unique_ptr<net::ChannelIDService> channel_id_service);
//     void SetHttpNetworkSession(
//         std::unique_ptr<net::HttpNetworkSession> http_network_session);
//     void SetHttpTransactionFactory(
//         std::unique_ptr<net::HttpTransactionFactory> http_factory);
//     void SetJobFactory(std::unique_ptr<net::URLRequestJobFactory> job_factory);

//    private:
//     ~AppRequestContext() override;

//     std::unique_ptr<net::CookieStore> cookie_store_;
//     std::unique_ptr<net::ChannelIDService> channel_id_service_;
//     std::unique_ptr<net::HttpNetworkSession> http_network_session_;
//     std::unique_ptr<net::HttpTransactionFactory> http_factory_;
//     std::unique_ptr<net::URLRequestJobFactory> job_factory_;
//   };

//   // Created on the UI thread, read on the IO thread during ProfileIOData lazy
//   // initialization.
//   struct ProfileParams {
//     ProfileParams();
//     ~ProfileParams();

//     base::FilePath path;
//     IOThread* io_thread;
//     scoped_refptr<content_settings::CookieSettings> cookie_settings;
//     scoped_refptr<HostContentSettingsMap> host_content_settings_map;
//     scoped_refptr<net::SSLConfigService> ssl_config_service;
//     scoped_refptr<net::CookieMonsterDelegate> cookie_monster_delegate;
// #if defined(ENABLE_EXTENSIONS)
//     scoped_refptr<extensions::InfoMap> extension_info_map;
// #endif
//     std::unique_ptr<chrome_browser_net::ResourcePrefetchPredictorObserver>
//         resource_prefetch_predictor_observer_;

//     // This pointer exists only as a means of conveying a url job factory
//     // pointer from the protocol handler registry on the UI thread to the
//     // the URLRequestContext on the IO thread. The consumer MUST take
//     // ownership of the object by calling release() on this pointer.
//     std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//         protocol_handler_interceptor;

//     // Holds the URLRequestInterceptor pointer that is created on the UI thread
//     // and then passed to the list of request_interceptors on the IO thread.
//     std::unique_ptr<net::URLRequestInterceptor> new_tab_page_interceptor;

//     // We need to initialize the ProxyConfigService from the UI thread
//     // because on linux it relies on initializing things through gconf,
//     // and needs to be on the main thread.
//     std::unique_ptr<net::ProxyConfigService> proxy_config_service;

// #if defined(ENABLE_SUPERVISED_USERS)
//     scoped_refptr<const SupervisedUserURLFilter> supervised_user_url_filter;
// #endif

// #if defined(OS_CHROMEOS)
//     std::string username_hash;
//     bool use_system_key_slot;
//     std::unique_ptr<chromeos::CertificateProvider> certificate_provider;
// #endif

//     // The profile this struct was populated from. It's passed as a void* to
//     // ensure it's not accidently used on the IO thread. Before using it on the
//     // UI thread, call ProfileManager::IsValidProfile to ensure it's alive.
//     void* profile;
//   };

//   explicit ProfileIOData(Profile::ProfileType profile_type);

//   void InitializeOnUIThread(Profile* profile);
//   void ApplyProfileParamsToContext(net::URLRequestContext* context) const;

//   std::unique_ptr<net::URLRequestJobFactory> SetUpJobFactoryDefaults(
//       std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory,
//       content::URLRequestInterceptorScopedVector request_interceptors,
//       std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//           protocol_handler_interceptor,
//       net::NetworkDelegate* network_delegate,
//       net::FtpTransactionFactory* ftp_transaction_factory) const;

//   // Called when the Profile is destroyed. |context_getters| must include all
//   // URLRequestContextGetters that refer to the ProfileIOData's
//   // URLRequestContexts. Triggers destruction of the ProfileIOData and shuts
//   // down |context_getters| safely on the IO thread.
//   // TODO(mmenke):  Passing all those URLRequestContextGetters around like this
//   //     is really silly.  Can we do something cleaner?
//   void ShutdownOnUIThread(
//       std::unique_ptr<ChromeURLRequestContextGetterVector> context_getters);

//   // A ChannelIDService object is created by a derived class of
//   // ProfileIOData, and the derived class calls this method to set the
//   // channel_id_service_ member and transfers ownership to the base
//   // class.
//   void set_channel_id_service(
//       net::ChannelIDService* channel_id_service) const;

//   void set_data_reduction_proxy_io_data(
//       std::unique_ptr<data_reduction_proxy::DataReductionProxyIOData>
//           data_reduction_proxy_io_data) const;

//   net::ProxyService* proxy_service() const {
//     return proxy_service_.get();
//   }

//   base::WeakPtr<net::HttpServerProperties> http_server_properties() const;

//   void set_http_server_properties(
//       std::unique_ptr<net::HttpServerProperties> http_server_properties) const;

//   net::URLRequestContext* main_request_context() const {
//     return main_request_context_.get();
//   }

//   bool initialized() const {
//     return initialized_;
//   }

//   // Destroys the ResourceContext first, to cancel any URLRequests that are
//   // using it still, before we destroy the member variables that those
//   // URLRequests may be accessing.
//   void DestroyResourceContext();

//   std::unique_ptr<net::HttpNetworkSession> CreateHttpNetworkSession(
//       const ProfileParams& profile_params) const;

//   // Creates main network transaction factory.
//   std::unique_ptr<net::HttpCache> CreateMainHttpFactory(
//       net::HttpNetworkSession* session,
//       std::unique_ptr<net::HttpCache::BackendFactory> main_backend) const;

//   // Creates network transaction factory.
//   std::unique_ptr<net::HttpCache> CreateHttpFactory(
//       net::HttpNetworkSession* shared_session,
//       std::unique_ptr<net::HttpCache::BackendFactory> backend) const;

//   void SetCookieSettingsForTesting(
//       content_settings::CookieSettings* cookie_settings);

 private:
//   class ResourceContext : public content::ResourceContext {
//    public:
//     explicit ResourceContext(ProfileIOData* io_data);
//     ~ResourceContext() override;

//     // ResourceContext implementation:
//     net::HostResolver* GetHostResolver() override;
//     net::URLRequestContext* GetRequestContext() override;
//     std::unique_ptr<net::ClientCertStore> CreateClientCertStore() override;
//     void CreateKeygenHandler(
//         uint32_t key_size_in_bits,
//         const std::string& challenge_string,
//         const GURL& url,
//         const base::Callback<void(std::unique_ptr<net::KeygenHandler>)>&
//             callback) override;
//     SaltCallback GetMediaDeviceIDSalt() override;

//    private:
//     friend class ProfileIOData;

//     ProfileIOData* const io_data_;

//     net::HostResolver* host_resolver_;
//     net::URLRequestContext* request_context_;
//   };

//   typedef std::map<StoragePartitionDescriptor,
//                    net::URLRequestContext*,
//                    StoragePartitionDescriptorLess>
//       URLRequestContextMap;

//   // --------------------------------------------
//   // Virtual interface for subtypes to implement:
//   // --------------------------------------------

//   // Does the actual initialization of the ProfileIOData subtype. Subtypes
//   // should use the static helper functions above to implement this.
//   virtual void InitializeInternal(
//       std::unique_ptr<ChromeNetworkDelegate> chrome_network_delegate,
//       ProfileParams* profile_params,
//       content::ProtocolHandlerMap* protocol_handlers,
//       content::URLRequestInterceptorScopedVector request_interceptors)
//       const = 0;

//   // Initializes the RequestContext for extensions.
//   virtual void InitializeExtensionsRequestContext(
//       ProfileParams* profile_params) const = 0;
//   // Does an on-demand initialization of a RequestContext for the given
//   // isolated app.
//   virtual net::URLRequestContext* InitializeAppRequestContext(
//       net::URLRequestContext* main_context,
//       const StoragePartitionDescriptor& details,
//       std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//           protocol_handler_interceptor,
//       content::ProtocolHandlerMap* protocol_handlers,
//       content::URLRequestInterceptorScopedVector request_interceptors)
//       const = 0;

//   // Does an on-demand initialization of a media RequestContext for the given
//   // isolated app.
//   virtual net::URLRequestContext* InitializeMediaRequestContext(
//       net::URLRequestContext* original_context,
//       const StoragePartitionDescriptor& details) const = 0;

//   // These functions are used to transfer ownership of the lazily initialized
//   // context from ProfileIOData to the URLRequestContextGetter.
//   virtual net::URLRequestContext*
//       AcquireMediaRequestContext() const = 0;
//   virtual net::URLRequestContext* AcquireIsolatedAppRequestContext(
//       net::URLRequestContext* main_context,
//       const StoragePartitionDescriptor& partition_descriptor,
//       std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
//           protocol_handler_interceptor,
//       content::ProtocolHandlerMap* protocol_handlers,
//       content::URLRequestInterceptorScopedVector request_interceptors)
//       const = 0;
//   virtual net::URLRequestContext*
//       AcquireIsolatedMediaRequestContext(
//           net::URLRequestContext* app_context,
//           const StoragePartitionDescriptor& partition_descriptor) const = 0;

//   // The order *DOES* matter for the majority of these member variables, so
//   // don't move them around unless you know what you're doing!
//   // General rules:
//   //   * ResourceContext references the URLRequestContexts, so
//   //   URLRequestContexts must outlive ResourceContext, hence ResourceContext
//   //   should be destroyed first.
//   //   * URLRequestContexts reference a whole bunch of members, so
//   //   URLRequestContext needs to be destroyed before them.
//   //   * Therefore, ResourceContext should be listed last, and then the
//   //   URLRequestContexts, and then the URLRequestContext members.
//   //   * Note that URLRequestContext members have a directed dependency graph
//   //   too, so they must themselves be ordered correctly.

//   // Tracks whether or not we've been lazily initialized.
//   mutable bool initialized_;

//   // Data from the UI thread from the Profile, used to initialize ProfileIOData.
//   // Deleted after lazy initialization.
//   mutable std::unique_ptr<ProfileParams> profile_params_;

//   // Used for testing.
//   mutable base::Callback<std::unique_ptr<net::ClientCertStore>()>
//       client_cert_store_factory_;

//   mutable StringPrefMember google_services_user_account_id_;

//   mutable scoped_refptr<MediaDeviceIDSalt> media_device_id_salt_;

//   // Member variables which are pointed to by the various context objects.
//   mutable BooleanPrefMember enable_referrers_;
//   mutable BooleanPrefMember enable_do_not_track_;
//   mutable BooleanPrefMember force_google_safesearch_;
//   mutable BooleanPrefMember force_youtube_safety_mode_;
//   mutable BooleanPrefMember safe_browsing_enabled_;
//   mutable BooleanPrefMember sync_disabled_;
//   mutable BooleanPrefMember signin_allowed_;
//   mutable IntegerPrefMember network_prediction_options_;
//   // TODO(marja): Remove session_startup_pref_ if no longer needed.
//   mutable IntegerPrefMember session_startup_pref_;
//   mutable BooleanPrefMember quick_check_enabled_;
//   mutable IntegerPrefMember incognito_availibility_pref_;

//   BooleanPrefMember enable_metrics_;

//   // Pointed to by NetworkDelegate.
//   mutable std::unique_ptr<policy::URLBlacklistManager> url_blacklist_manager_;
//   mutable std::unique_ptr<policy::PolicyHeaderIOHelper> policy_header_helper_;

//   // Pointed to by URLRequestContext.
// #if defined(ENABLE_EXTENSIONS)
//   mutable scoped_refptr<extensions::InfoMap> extension_info_map_;
// #endif
//   mutable std::unique_ptr<net::ChannelIDService> channel_id_service_;

//   mutable std::unique_ptr<data_reduction_proxy::DataReductionProxyIOData>
//       data_reduction_proxy_io_data_;

//   mutable std::unique_ptr<net::ProxyService> proxy_service_;
//   mutable std::unique_ptr<net::TransportSecurityState>
//       transport_security_state_;
//   mutable std::unique_ptr<net::CTVerifier> cert_transparency_verifier_;
//   mutable std::unique_ptr<ChromeExpectCTReporter> expect_ct_reporter_;
//   mutable std::unique_ptr<net::HttpServerProperties> http_server_properties_;
// #if defined(OS_CHROMEOS)
//   // Set to |cert_verifier_| if it references a PolicyCertVerifier. In that
//   // case, the verifier is owned by  |cert_verifier_|. Otherwise, set to NULL.
//   mutable std::unique_ptr<net::CertVerifier> cert_verifier_;
//   mutable policy::PolicyCertVerifier* policy_cert_verifier_;
//   mutable std::string username_hash_;
//   mutable bool use_system_key_slot_;
//   mutable std::unique_ptr<chromeos::CertificateProvider> certificate_provider_;
// #endif

//   mutable std::unique_ptr<net::TransportSecurityPersister>
//       transport_security_persister_;
//   mutable std::unique_ptr<net::CertificateReportSender>
//       certificate_report_sender_;

//   // These are only valid in between LazyInitialize() and their accessor being
//   // called.
//   mutable std::unique_ptr<net::URLRequestContext> main_request_context_;
//   mutable std::unique_ptr<net::URLRequestContext> extensions_request_context_;
//   // One URLRequestContext per isolated app for main and media requests.
//   mutable URLRequestContextMap app_request_context_map_;
//   mutable URLRequestContextMap isolated_media_request_context_map_;

//   mutable std::unique_ptr<ResourceContext> resource_context_;

//   mutable scoped_refptr<content_settings::CookieSettings> cookie_settings_;

//   mutable scoped_refptr<HostContentSettingsMap> host_content_settings_map_;

//   mutable std::unique_ptr<chrome_browser_net::ResourcePrefetchPredictorObserver>
//       resource_prefetch_predictor_observer_;

//   mutable std::unique_ptr<ChromeHttpUserAgentSettings>
//       chrome_http_user_agent_settings_;

// #if defined(ENABLE_SUPERVISED_USERS)
//   mutable scoped_refptr<const SupervisedUserURLFilter>
//       supervised_user_url_filter_;
// #endif

// #if defined(ENABLE_EXTENSIONS)
//   // Is NULL if switches::kDisableExtensionsHttpThrottling is on.
//   mutable std::unique_ptr<extensions::ExtensionThrottleManager>
//       extension_throttle_manager_;
// #endif

//   mutable DevToolsNetworkControllerHandle network_controller_handle_;

//   const Profile::ProfileType profile_type_;

  DISALLOW_COPY_AND_ASSIGN(ProfileIOData);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_IO_DATA_H_
