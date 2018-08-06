// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_context.h"

#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/atom_blob_reader.h"
#include "atom/browser/atom_download_manager_delegate.h"
#include "atom/browser/atom_permission_manager.h"
#include "atom/browser/browser.h"
#include "atom/browser/net/request_context_factory.h"
#include "atom/browser/web_view_manager.h"
#include "atom/common/atom_version.h"
#include "atom/common/chrome_version.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/common/user_agent.h"

using content::BrowserThread;

namespace atom {

namespace {

std::string RemoveWhitespace(const std::string& str) {
  std::string trimmed;
  if (base::RemoveChars(str, " ", &trimmed))
    return trimmed;
  else
    return str;
}

std::vector<std::string> GetCookieableSchemes() {
  std::vector<std::string> default_schemes = {"http", "https", "ws", "wss"};
  const auto& standard_schemes = atom::api::GetStandardSchemes();
  default_schemes.insert(default_schemes.end(), standard_schemes.begin(),
                         standard_schemes.end());
  return default_schemes;
}

}  // namespace

AtomBrowserContext::AtomBrowserContext(const std::string& partition,
                                       bool in_memory,
                                       const base::DictionaryValue& options)
    : brightray::BrowserContext(partition, in_memory) {
  // Construct user agent string.
  Browser* browser = Browser::Get();
  std::string name = RemoveWhitespace(browser->GetName());
  std::string user_agent;
  if (name == ATOM_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " " ATOM_PRODUCT_NAME
                 "/" ATOM_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING,
        name.c_str(), browser->GetVersion().c_str(), CHROME_VERSION_STRING);
  }
  user_agent_ = content::BuildUserAgentFromProduct(user_agent);

  // Read options.
  use_cache_ = true;
  options.GetBoolean("cache", &use_cache_);

  // Initialize Pref Registry in brightray.
  InitPrefs();
}

AtomBrowserContext::~AtomBrowserContext() {}

void AtomBrowserContext::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
AtomBrowserContext::RegisterCookieChangeCallback(
    const base::Callback<void(const CookieDetails*)>& cb) {
  return cookie_change_sub_list_.Add(cb);
}

std::string AtomBrowserContext::GetUserAgent() const {
  return user_agent_;
}

content::DownloadManagerDelegate*
AtomBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto* download_manager = content::BrowserContext::GetDownloadManager(this);
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

void AtomBrowserContext::RegisterPrefs(PrefRegistrySimple* pref_registry) {
  pref_registry->RegisterFilePathPref(prefs::kSelectFileLastDirectory,
                                      base::FilePath());
  base::FilePath download_dir;
  PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &download_dir);
  pref_registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                      download_dir);
  pref_registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
}

brightray::URLRequestContextGetterFactory*
AtomBrowserContext::GetFactoryForMainRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return new AtomMainRequestContextFactory(
      GetPath(), IsOffTheRecord(), use_cache_, user_agent_,
      GetCookieableSchemes(), protocol_handlers,
      std::move(request_interceptors), this);
}

AtomBlobReader* AtomBrowserContext::GetBlobReader() {
  if (!blob_reader_.get()) {
    content::ChromeBlobStorageContext* blob_context =
        content::ChromeBlobStorageContext::GetFor(this);
    blob_reader_.reset(new AtomBlobReader(blob_context));
  }
  return blob_reader_.get();
}

void AtomBrowserContext::NotifyCookieChange(
    const CookieDetails* cookie_details) {
  cookie_change_sub_list_.Notify(cookie_details);
}

// static
scoped_refptr<AtomBrowserContext> AtomBrowserContext::From(
    const std::string& partition,
    bool in_memory,
    const base::DictionaryValue& options) {
  auto browser_context = brightray::BrowserContext::Get(partition, in_memory);
  if (browser_context)
    return static_cast<AtomBrowserContext*>(browser_context.get());

  return new AtomBrowserContext(partition, in_memory, options);
}

}  // namespace atom
