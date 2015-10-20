// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_context.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_download_manager_delegate.h"
#include "atom/browser/atom_ssl_config_service.h"
#include "atom/browser/browser.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "atom/browser/net/asar/asar_protocol_handler.h"
#include "atom/browser/net/http_protocol_handler.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
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

namespace atom {

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

AtomBrowserContext::AtomBrowserContext(const std::string& partition,
                                       bool in_memory)
    : brightray::BrowserContext(partition, in_memory),
      job_factory_(new AtomURLRequestJobFactory),
      allow_ntlm_everywhere_(false) {
}

AtomBrowserContext::~AtomBrowserContext() {
}

std::string AtomBrowserContext::GetUserAgent() {
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
  return content::BuildUserAgentFromProduct(user_agent);
}

net::URLRequestJobFactory* AtomBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* handlers,
    content::URLRequestInterceptorScopedVector* interceptors) {
  scoped_ptr<AtomURLRequestJobFactory> job_factory(job_factory_);

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
  scoped_ptr<net::URLRequestJobFactory> top_job_factory = job_factory.Pass();
  content::URLRequestInterceptorScopedVector::reverse_iterator it;
  for (it = interceptors->rbegin(); it != interceptors->rend(); ++it)
    top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
        top_job_factory.Pass(), make_scoped_ptr(*it)));
  interceptors->weak_clear();

  return top_job_factory.release();
}

net::HttpCache::BackendFactory*
AtomBrowserContext::CreateHttpCacheBackendFactory(
    const base::FilePath& base_path) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisableHttpCache))
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

net::SSLConfigService* AtomBrowserContext::CreateSSLConfigService() {
  return new AtomSSLConfigService;
}

void AtomBrowserContext::RegisterPrefs(PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                      base::FilePath());
  pref_registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                      base::FilePath());
}

bool AtomBrowserContext::AllowNTLMCredentialsForDomain(const GURL& origin) {
  if (allow_ntlm_everywhere_)
    return true;
  return Delegate::AllowNTLMCredentialsForDomain(origin);
}

void AtomBrowserContext::AllowNTLMCredentialsForAllDomains(bool should_allow) {
  allow_ntlm_everywhere_ = should_allow;
}

}  // namespace atom

namespace brightray {

// static
scoped_refptr<BrowserContext> BrowserContext::Create(
    const std::string& partition, bool in_memory) {
  return make_scoped_refptr(new atom::AtomBrowserContext(partition, in_memory));
}

}  // namespace brightray
