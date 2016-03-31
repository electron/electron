// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/atom_browser_context.h"

#include "electron/browser/atom_browser_main_parts.h"
#include "electron/browser/atom_download_manager_delegate.h"
#include "electron/browser/browser.h"
#include "electron/browser/net/atom_cert_verifier.h"
#include "electron/browser/net/atom_network_delegate.h"
#include "electron/browser/net/atom_ssl_config_service.h"
#include "electron/browser/net/atom_url_request_job_factory.h"
#include "electron/browser/net/asar/asar_protocol_handler.h"
#include "electron/browser/net/http_protocol_handler.h"
#include "electron/browser/atom_permission_manager.h"
#include "electron/browser/web_view_manager.h"
#include "electron/common/atom_version.h"
#include "electron/common/chrome_version.h"
#include "electron/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "net/ftp/ftp_network_layer.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_context.h"
#include "url/url_constants.h"

using content::BrowserThread;

namespace electron {

namespace {

class NoCacheBackend : public net::HttpCache::BackendFactory {
  int CreateBackend(net::NetLog* net_log,
                    scoped_ptr<disk_cache::Backend>* backend,
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

ElectronBrowserContext::ElectronBrowserContext(const std::string& partition,
                                       bool in_memory)
    : brightray::BrowserContext(partition, in_memory),
      cert_verifier_(nullptr),
      job_factory_(new ElectronURLRequestJobFactory),
      network_delegate_(new ElectronNetworkDelegate),
      allow_ntlm_everywhere_(false) {
}

ElectronBrowserContext::~ElectronBrowserContext() {
}

net::NetworkDelegate* ElectronBrowserContext::CreateNetworkDelegate() {
  return network_delegate_;
}

std::string ElectronBrowserContext::GetUserAgent() {
  Browser* browser = Browser::Get();
  std::string name = RemoveWhitespace(browser->GetName());
  std::string user_agent;
  if (name == ELECTRON_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " "
                 ELECTRON_PRODUCT_NAME "/" ELECTRON_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ELECTRON_PRODUCT_NAME "/" ELECTRON_VERSION_STRING,
        name.c_str(),
        browser->GetVersion().c_str(),
        CHROME_VERSION_STRING);
  }
  return content::BuildUserAgentFromProduct(user_agent);
}

scoped_ptr<net::URLRequestJobFactory>
ElectronBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* handlers,
    content::URLRequestInterceptorScopedVector* interceptors) {
  scoped_ptr<ElectronURLRequestJobFactory> job_factory(job_factory_);

  for (auto& it : *handlers) {
    job_factory->SetProtocolHandler(it.first,
                                    make_scoped_ptr(it.second.release()));
  }
  handlers->clear();

  job_factory->SetProtocolHandler(
      url::kDataScheme, make_scoped_ptr(new net::DataProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kFileScheme, make_scoped_ptr(new asar::AsarProtocolHandler(
          BrowserThread::GetBlockingPool()->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN))));
  job_factory->SetProtocolHandler(
      url::kHttpScheme,
      make_scoped_ptr(new HttpProtocolHandler(url::kHttpScheme)));
  job_factory->SetProtocolHandler(
      url::kHttpsScheme,
      make_scoped_ptr(new HttpProtocolHandler(url::kHttpsScheme)));
  job_factory->SetProtocolHandler(
      url::kWsScheme,
      make_scoped_ptr(new HttpProtocolHandler(url::kWsScheme)));
  job_factory->SetProtocolHandler(
      url::kWssScheme,
      make_scoped_ptr(new HttpProtocolHandler(url::kWssScheme)));

  auto host_resolver =
      url_request_context_getter()->GetURLRequestContext()->host_resolver();
  job_factory->SetProtocolHandler(
      url::kFtpScheme,
      make_scoped_ptr(new net::FtpProtocolHandler(
          new net::FtpNetworkLayer(host_resolver))));

  // Set up interceptors in the reverse order.
  scoped_ptr<net::URLRequestJobFactory> top_job_factory =
      std::move(job_factory);
  content::URLRequestInterceptorScopedVector::reverse_iterator it;
  for (it = interceptors->rbegin(); it != interceptors->rend(); ++it)
    top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
        std::move(top_job_factory), make_scoped_ptr(*it)));
  interceptors->weak_clear();

  return top_job_factory;
}

net::HttpCache::BackendFactory*
ElectronBrowserContext::CreateHttpCacheBackendFactory(
    const base::FilePath& base_path) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisableHttpCache))
    return new NoCacheBackend;
  else
    return brightray::BrowserContext::CreateHttpCacheBackendFactory(base_path);
}

content::DownloadManagerDelegate*
ElectronBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto download_manager = content::BrowserContext::GetDownloadManager(this);
    download_manager_delegate_.reset(
        new ElectronDownloadManagerDelegate(download_manager));
  }
  return download_manager_delegate_.get();
}

content::BrowserPluginGuestManager* ElectronBrowserContext::GetGuestManager() {
  if (!guest_manager_)
    guest_manager_.reset(new WebViewManager);
  return guest_manager_.get();
}

content::PermissionManager* ElectronBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new ElectronPermissionManager);
  return permission_manager_.get();
}

scoped_ptr<net::CertVerifier> ElectronBrowserContext::CreateCertVerifier() {
  DCHECK(!cert_verifier_);
  cert_verifier_ = new ElectronCertVerifier;
  return make_scoped_ptr(cert_verifier_);
}

net::SSLConfigService* ElectronBrowserContext::CreateSSLConfigService() {
  return new ElectronSSLConfigService;
}

void ElectronBrowserContext::RegisterPrefs(PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                      base::FilePath());
  base::FilePath download_dir;
  PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &download_dir);
  pref_registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                      download_dir);
  pref_registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
}

bool ElectronBrowserContext::AllowNTLMCredentialsForDomain(const GURL& origin) {
  if (allow_ntlm_everywhere_)
    return true;
  return Delegate::AllowNTLMCredentialsForDomain(origin);
}

void ElectronBrowserContext::AllowNTLMCredentialsForAllDomains(bool should_allow) {
  allow_ntlm_everywhere_ = should_allow;
}

}  // namespace electron

namespace brightray {

// static
scoped_refptr<BrowserContext> BrowserContext::Create(
    const std::string& partition, bool in_memory) {
  return make_scoped_refptr(new electron::ElectronBrowserContext(partition, in_memory));
}

}  // namespace brightray
