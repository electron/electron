// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/request_context_delegate.h"

#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/net/about_protocol_handler.h"
#include "atom/browser/net/asar/asar_protocol_handler.h"
#include "atom/browser/net/atom_cert_verifier.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "atom/browser/net/cookie_details.h"
#include "atom/browser/net/http_protocol_handler.h"
#include "atom/common/options_switches.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
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

}  // namespace

RequestContextDelegate::RequestContextDelegate(bool use_cache)
    : use_cache_(use_cache), weak_factory_(this) {}

RequestContextDelegate::~RequestContextDelegate() {}

std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
RequestContextDelegate::RegisterCookieChangeCallback(
    const base::Callback<void(const CookieDetails*)>& cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return cookie_change_sub_list_.Add(cb);
}

void RequestContextDelegate::NotifyCookieChange(
    const net::CanonicalCookie& cookie,
    net::CookieChangeCause cause) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  CookieDetails cookie_details(
      &cookie, !(cause == net::CookieChangeCause::INSERTED), cause);
  cookie_change_sub_list_.Notify(&cookie_details);
}

std::unique_ptr<net::NetworkDelegate>
RequestContextDelegate::CreateNetworkDelegate() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  return std::make_unique<AtomNetworkDelegate>();
}

std::unique_ptr<net::URLRequestJobFactory>
RequestContextDelegate::CreateURLRequestJobFactory(
    net::URLRequestContext* url_request_context,
    content::ProtocolHandlerMap* protocol_handlers) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<AtomURLRequestJobFactory> job_factory(
      new AtomURLRequestJobFactory);

  for (auto& it : *protocol_handlers) {
    job_factory->SetProtocolHandler(it.first,
                                    base::WrapUnique(it.second.release()));
  }
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(url::kAboutScheme,
                                  base::WrapUnique(new AboutProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kFileScheme,
      base::WrapUnique(
          new asar::AsarProtocolHandler(base::CreateTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
               base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}))));
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

  auto* host_resolver = url_request_context->host_resolver();
  job_factory->SetProtocolHandler(
      url::kFtpScheme, net::FtpProtocolHandler::Create(host_resolver));

  return std::move(job_factory);
}

net::HttpCache::BackendFactory*
RequestContextDelegate::CreateHttpCacheBackendFactory(
    const base::FilePath& base_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!use_cache_) {
    return new NoCacheBackend;
  } else {
    int max_size = 0;
    base::StringToInt(
        command_line->GetSwitchValueASCII(switches::kDiskCacheSize), &max_size);

    base::FilePath cache_path = base_path.Append(FILE_PATH_LITERAL("Cache"));
    return new net::HttpCache::DefaultBackend(
        net::DISK_CACHE, net::CACHE_BACKEND_DEFAULT, cache_path, max_size);
  }
}

std::unique_ptr<net::CertVerifier> RequestContextDelegate::CreateCertVerifier(
    brightray::RequireCTDelegate* ct_delegate) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  return std::make_unique<AtomCertVerifier>(ct_delegate);
}

void RequestContextDelegate::GetCookieableSchemes(
    std::vector<std::string>* cookie_schemes) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const auto& standard_schemes = atom::api::GetStandardSchemes();
  cookie_schemes->insert(cookie_schemes->end(), standard_schemes.begin(),
                         standard_schemes.end());
}

void RequestContextDelegate::OnCookieChanged(const net::CanonicalCookie& cookie,
                                             net::CookieChangeCause cause) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindRepeating(&RequestContextDelegate::NotifyCookieChange,
                          weak_factory_.GetWeakPtr(), cookie, cause));
}

}  // namespace atom
