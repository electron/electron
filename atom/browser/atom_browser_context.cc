// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_context.h"

#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/atom_blob_reader.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_download_manager_delegate.h"
#include "atom/browser/atom_permission_manager.h"
#include "atom/browser/browser.h"
#include "atom/browser/net/asar/asar_protocol_handler.h"
#include "atom/browser/net/atom_cert_verifier.h"
#include "atom/browser/net/atom_ct_delegate.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/net/atom_ssl_config_service.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "atom/browser/net/http_protocol_handler.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "net/ftp/ftp_network_layer.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "url/url_constants.h"

using content::BrowserThread;

namespace atom {

namespace {

class NoCacheBackend : public net::HttpCache::BackendFactory {
  int CreateBackend(net::NetLog* net_log,
                    std::unique_ptr<disk_cache::Backend>* backend,
                    const net::CompletionCallback& callback) override {
    return net::ERR_FAILED;
  }
};

std::string RemoveWhitespace(const std::string& str) {
  std::string trimmed;
  if (base::RemoveChars(str, " ", &trimmed))
    return trimmed;
  else
    return str;
}

}  // namespace

AtomBrowserContext::AtomBrowserContext(const std::string& partition,
                                       bool in_memory,
                                       const base::DictionaryValue& options)
    : brightray::BrowserContext(partition, in_memory),
      ct_delegate_(new AtomCTDelegate),
      network_delegate_(new AtomNetworkDelegate),
      cookie_delegate_(new AtomCookieDelegate) {
  // Construct user agent string.
  Browser* browser = Browser::Get();
  std::string name = RemoveWhitespace(browser->GetName());
  std::string user_agent;
  if (name == ATOM_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " "
                 ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING,
        name.c_str(),
        browser->GetVersion().c_str(),
        CHROME_VERSION_STRING);
  }
  user_agent_ = content::BuildUserAgentFromProduct(user_agent);

  // Read options.
  use_cache_ = true;
  options.GetBoolean("cache", &use_cache_);

  // Initialize Pref Registry in brightray.
  InitPrefs();
}

AtomBrowserContext::~AtomBrowserContext() {
}

void AtomBrowserContext::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

net::NetworkDelegate* AtomBrowserContext::CreateNetworkDelegate() {
  return network_delegate_;
}

net::CookieMonsterDelegate* AtomBrowserContext::CreateCookieDelegate() {
  return cookie_delegate();
}

std::string AtomBrowserContext::GetUserAgent() {
  return user_agent_;
}

std::unique_ptr<net::URLRequestJobFactory>
AtomBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers) {
  std::unique_ptr<AtomURLRequestJobFactory> job_factory(
      new AtomURLRequestJobFactory);

  for (auto& it : *protocol_handlers) {
    job_factory->SetProtocolHandler(it.first,
                                    base::WrapUnique(it.second.release()));
  }
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(
      url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kFileScheme, base::WrapUnique(new asar::AsarProtocolHandler(
          BrowserThread::GetBlockingPool()->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN))));
  job_factory->SetProtocolHandler(
      url::kHttpScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kHttpScheme)));
  job_factory->SetProtocolHandler(
      url::kHttpsScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kHttpsScheme)));
  job_factory->SetProtocolHandler(
      url::kWsScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kWsScheme)));
  job_factory->SetProtocolHandler(
      url::kWssScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kWssScheme)));

  auto host_resolver =
      url_request_context_getter()->GetURLRequestContext()->host_resolver();
  job_factory->SetProtocolHandler(
      url::kFtpScheme,
      base::WrapUnique(new net::FtpProtocolHandler(
          new net::FtpNetworkLayer(host_resolver))));

  return std::move(job_factory);
}

net::HttpCache::BackendFactory*
AtomBrowserContext::CreateHttpCacheBackendFactory(
    const base::FilePath& base_path) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!use_cache_ || command_line->HasSwitch(switches::kDisableHttpCache))
    return new NoCacheBackend;
  else
    return brightray::BrowserContext::CreateHttpCacheBackendFactory(base_path);
}

content::DownloadManagerDelegate*
AtomBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto download_manager = content::BrowserContext::GetDownloadManager(this);
    download_manager_delegate_.reset(
        new AtomDownloadManagerDelegate(download_manager));
  }
  return download_manager_delegate_.get();
}

content::BrowserPluginGuestManager* AtomBrowserContext::GetGuestManager() {
  if (!guest_manager_)
    guest_manager_.reset(new WebViewManager);
  return guest_manager_.get();
}

content::PermissionManager* AtomBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new AtomPermissionManager);
  return permission_manager_.get();
}

std::unique_ptr<net::CertVerifier> AtomBrowserContext::CreateCertVerifier() {
  return base::WrapUnique(new AtomCertVerifier());
}

net::SSLConfigService* AtomBrowserContext::CreateSSLConfigService() {
  return new AtomSSLConfigService;
}

std::vector<std::string> AtomBrowserContext::GetCookieableSchemes() {
  auto default_schemes = brightray::BrowserContext::GetCookieableSchemes();
  const auto& standard_schemes = atom::api::GetStandardSchemes();
  default_schemes.insert(default_schemes.end(),
                         standard_schemes.begin(), standard_schemes.end());
  return default_schemes;
}

net::TransportSecurityState::RequireCTDelegate*
AtomBrowserContext::GetRequireCTDelegate() {
  return ct_delegate_.get();
}

void AtomBrowserContext::RegisterPrefs(PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                      base::FilePath());
  base::FilePath download_dir;
  PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &download_dir);
  pref_registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                      download_dir);
  pref_registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
}

AtomBlobReader* AtomBrowserContext::GetBlobReader() {
  if (!blob_reader_.get()) {
    content::ChromeBlobStorageContext* blob_context =
        content::ChromeBlobStorageContext::GetFor(this);
    storage::FileSystemContext* file_system_context =
        content::BrowserContext::GetStoragePartition(
            this, nullptr)->GetFileSystemContext();
    blob_reader_.reset(new AtomBlobReader(blob_context,
                                          file_system_context));
  }
  return blob_reader_.get();
}

// static
scoped_refptr<AtomBrowserContext> AtomBrowserContext::From(
    const std::string& partition, bool in_memory,
    const base::DictionaryValue& options) {
  auto browser_context = brightray::BrowserContext::Get(partition, in_memory);
  if (browser_context)
    return static_cast<AtomBrowserContext*>(browser_context.get());

  return new AtomBrowserContext(partition, in_memory, options);
}

}  // namespace atom
