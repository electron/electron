// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This is not a straight copy from chromium src, in particular
 * some functionality is removed so that it doesn't use shell
 * handling, which we handle separately in electron code.
 * ProtocolHandlerRegistry::TranslateUrl was also added.
 * This was originally forked with 51.0.2704.106.  Diff against
 * a version of that file for a full list of changes.
 */

#include "chrome/browser/custom_handlers/protocol_handler_registry.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
//#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/custom_handlers/protocol_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/child_process_security_policy.h"
#include "net/base/network_delegate.h"
#include "net/url_request/url_request_redirect_job.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;
using content::ChildProcessSecurityPolicy;

namespace {

const ProtocolHandler& LookupHandler(
    const ProtocolHandlerRegistry::ProtocolHandlerMap& handler_map,
    const std::string& scheme) {
  ProtocolHandlerRegistry::ProtocolHandlerMap::const_iterator p =
      handler_map.find(scheme);

  if (p != handler_map.end())
    return p->second;

  return ProtocolHandler::EmptyProtocolHandler();
}

// If true default protocol handlers will be removed if the OS level
// registration for a protocol is no longer Chrome.
bool ShouldRemoveHandlersNotInOS() {
#if defined(OS_LINUX)
  // We don't do this on Linux as the OS registration there is not reliable,
  // and Chrome OS doesn't have any notion of OS registration.
  // TODO(benwells): When Linux support is more reliable remove this
  // difference (http://crbug.com/88255).
  return false;
#else
  return true;
#endif
}

}  // namespace

// IOThreadDelegate ------------------------------------------------------------

// IOThreadDelegate is an IO thread specific object. Access to the class should
// all be done via the IO thread. The registry living on the UI thread makes
// a best effort to update the IO object after local updates are completed.
class ProtocolHandlerRegistry::IOThreadDelegate
    : public base::RefCountedThreadSafe<
          ProtocolHandlerRegistry::IOThreadDelegate> {
 public:
  // Creates a new instance. If |enabled| is true the registry is considered
  // enabled on the IO thread.
  explicit IOThreadDelegate(bool enabled);

  // Returns true if the protocol has a default protocol handler.
  // Should be called only from the IO thread.
  bool IsHandledProtocol(const std::string& scheme) const;

  // Clears the default for the provided protocol.
  // Should be called only from the IO thread.
  void ClearDefault(const std::string& scheme);

  // Makes this ProtocolHandler the default handler for its protocol.
  // Should be called only from the IO thread.
  void SetDefault(const ProtocolHandler& handler);

  // Creates a URL request job for the given request if there is a matching
  // protocol handler, returns NULL otherwise.
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request, net::NetworkDelegate* network_delegate) const;

  // Indicate that the registry has been enabled in the IO thread's
  // copy of the data.
  void Enable() { enabled_ = true; }

  // Indicate that the registry has been disabled in the IO thread's copy of
  // the data.
  void Disable() { enabled_ = false; }

 private:
  friend class base::RefCountedThreadSafe<IOThreadDelegate>;
  virtual ~IOThreadDelegate();

  // Copy of protocol handlers use only on the IO thread.
  ProtocolHandlerRegistry::ProtocolHandlerMap default_handlers_;

  // Is the registry enabled on the IO thread.
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(IOThreadDelegate);
};

ProtocolHandlerRegistry::IOThreadDelegate::IOThreadDelegate(bool)
    : enabled_(true) {}
ProtocolHandlerRegistry::IOThreadDelegate::~IOThreadDelegate() {}

bool ProtocolHandlerRegistry::IOThreadDelegate::IsHandledProtocol(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return enabled_ && !LookupHandler(default_handlers_, scheme).IsEmpty();
}

void ProtocolHandlerRegistry::IOThreadDelegate::ClearDefault(
    const std::string& scheme) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  default_handlers_.erase(scheme);
}

void ProtocolHandlerRegistry::IOThreadDelegate::SetDefault(
    const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ClearDefault(handler.protocol());
  default_handlers_.insert(std::make_pair(handler.protocol(), handler));
}

// Create a new job for the supplied |URLRequest| if a default handler
// is registered and the associated handler is able to interpret
// the url from |request|.
net::URLRequestJob* ProtocolHandlerRegistry::IOThreadDelegate::MaybeCreateJob(
    net::URLRequest* request, net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ProtocolHandler handler = LookupHandler(default_handlers_,
                                          request->url().scheme());
  if (handler.IsEmpty())
    return NULL;

  GURL translated_url(handler.TranslateUrl(request->url()));
  if (!translated_url.is_valid())
    return NULL;

  return new net::URLRequestRedirectJob(
      request, network_delegate, translated_url,
      net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
      "Protocol Handler Registry");
}

// JobInterceptorFactory -------------------------------------------------------

// Instances of JobInterceptorFactory are produced for ownership by the IO
// thread where it handler URL requests. We should never hold
// any pointers on this class, only produce them in response to
// requests via |ProtocolHandlerRegistry::CreateJobInterceptorFactory|.
ProtocolHandlerRegistry::JobInterceptorFactory::JobInterceptorFactory(
    IOThreadDelegate* io_thread_delegate)
    : io_thread_delegate_(io_thread_delegate) {
  DCHECK(io_thread_delegate_.get());
  DetachFromThread();
}

ProtocolHandlerRegistry::JobInterceptorFactory::~JobInterceptorFactory() {
}

void ProtocolHandlerRegistry::JobInterceptorFactory::Chain(
    scoped_ptr<net::URLRequestJobFactory> job_factory) {
  job_factory_ = std::move(job_factory);
}

net::URLRequestJob*
ProtocolHandlerRegistry::JobInterceptorFactory::
MaybeCreateJobWithProtocolHandler(
    const std::string& scheme,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  net::URLRequestJob* job = io_thread_delegate_->MaybeCreateJob(
      request, network_delegate);
  if (job)
    return job;
  return job_factory_->MaybeCreateJobWithProtocolHandler(
      scheme, request, network_delegate);
}

net::URLRequestJob*
ProtocolHandlerRegistry::JobInterceptorFactory::MaybeInterceptRedirect(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const GURL& location) const {
  return job_factory_->MaybeInterceptRedirect(
      request, network_delegate, location);
}

net::URLRequestJob*
ProtocolHandlerRegistry::JobInterceptorFactory::MaybeInterceptResponse(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return job_factory_->MaybeInterceptResponse(request, network_delegate);
}

bool ProtocolHandlerRegistry::JobInterceptorFactory::IsHandledProtocol(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return io_thread_delegate_->IsHandledProtocol(scheme) ||
      job_factory_->IsHandledProtocol(scheme);
}

bool ProtocolHandlerRegistry::JobInterceptorFactory::IsHandledURL(
    const GURL& url) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return (url.is_valid() &&
      io_thread_delegate_->IsHandledProtocol(url.scheme())) ||
      job_factory_->IsHandledURL(url);
}

bool ProtocolHandlerRegistry::JobInterceptorFactory::IsSafeRedirectTarget(
    const GURL& location) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return job_factory_->IsSafeRedirectTarget(location);
}

// Delegate --------------------------------------------------------------------

ProtocolHandlerRegistry::Delegate::~Delegate() {}

void ProtocolHandlerRegistry::Delegate::RegisterExternalHandler(
    const std::string& protocol) {
  ChildProcessSecurityPolicy* policy =
    ChildProcessSecurityPolicy::GetInstance();
  if (!policy->IsWebSafeScheme(protocol)) {
    policy->RegisterWebSafeScheme(protocol);
  }
}

void ProtocolHandlerRegistry::Delegate::DeregisterExternalHandler(
    const std::string& protocol) {
}

bool ProtocolHandlerRegistry::Delegate::IsExternalHandlerRegistered(
    const std::string& protocol) {
  // NOTE(koz): This function is safe to call from any thread, despite living
  // in ProfileIOData.
  // return ProfileIOData::IsHandledProtocol(protocol);
  return false;
}

// ProtocolHandlerRegistry -----------------------------------------------------

ProtocolHandlerRegistry::ProtocolHandlerRegistry(
    content::BrowserContext* context,
    Delegate* delegate)
    : context_(context),
      delegate_(delegate),
      enabled_(true),
      is_loading_(false),
      is_loaded_(false),
      io_thread_delegate_(new IOThreadDelegate(enabled_)),
      weak_ptr_factory_(this) {}

bool ProtocolHandlerRegistry::SilentlyHandleRegisterHandlerRequest(
    const ProtocolHandler& handler) {
  if (handler.IsEmpty() || !CanSchemeBeOverridden(handler.protocol()))
    return true;

  if (!enabled() || IsRegistered(handler) || HasIgnoredEquivalent(handler))
    return true;

  if (AttemptReplace(handler))
    return true;

  return false;
}

void ProtocolHandlerRegistry::OnAcceptRegisterProtocolHandler(
    const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RegisterProtocolHandler(handler, USER);
  SetDefault(handler);
  Save();
  NotifyChanged();
}

void ProtocolHandlerRegistry::OnDenyRegisterProtocolHandler(
    const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RegisterProtocolHandler(handler, USER);
  Save();
  NotifyChanged();
}

void ProtocolHandlerRegistry::OnIgnoreRegisterProtocolHandler(
    const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  IgnoreProtocolHandler(handler, USER);
  Save();
  NotifyChanged();
}

bool ProtocolHandlerRegistry::AttemptReplace(const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandler old_default = GetHandlerFor(handler.protocol());
  bool make_new_handler_default = handler.IsSameOrigin(old_default);
  ProtocolHandlerList to_replace(GetReplacedHandlers(handler));
  if (to_replace.empty())
    return false;
  for (ProtocolHandlerList::iterator p = to_replace.begin();
       p != to_replace.end(); ++p) {
    RemoveHandler(*p);
  }
  if (make_new_handler_default) {
    OnAcceptRegisterProtocolHandler(handler);
  } else {
    InsertHandler(handler);
    NotifyChanged();
  }
  return true;
}

ProtocolHandlerRegistry::ProtocolHandlerList
ProtocolHandlerRegistry::GetReplacedHandlers(
    const ProtocolHandler& handler) const {
  ProtocolHandlerList replaced_handlers;
  const ProtocolHandlerList* handlers = GetHandlerList(handler.protocol());
  if (!handlers)
    return replaced_handlers;
  for (ProtocolHandlerList::const_iterator p = handlers->begin();
       p != handlers->end(); p++) {
    if (handler.IsSameOrigin(*p)) {
      replaced_handlers.push_back(*p);
    }
  }
  return replaced_handlers;
}

void ProtocolHandlerRegistry::ClearDefault(const std::string& scheme) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  default_handlers_.erase(scheme);
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&IOThreadDelegate::ClearDefault, io_thread_delegate_, scheme));
  Save();
  NotifyChanged();
}

bool ProtocolHandlerRegistry::IsDefault(
    const ProtocolHandler& handler) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return GetHandlerFor(handler.protocol()) == handler;
}

void ProtocolHandlerRegistry::InstallDefaultsForChromeOS() {
#if defined(OS_CHROMEOS)
  // Only chromeos has default protocol handlers at this point.
  AddPredefinedHandler(
      ProtocolHandler::CreateProtocolHandler(
          "mailto",
          GURL(l10n_util::GetStringUTF8(IDS_GOOGLE_MAILTO_HANDLER_URL))));
  AddPredefinedHandler(
      ProtocolHandler::CreateProtocolHandler(
          "webcal",
          GURL(l10n_util::GetStringUTF8(IDS_GOOGLE_WEBCAL_HANDLER_URL))));
#else
  NOTREACHED();  // this method should only ever be called in chromeos.
#endif
}

void ProtocolHandlerRegistry::InitProtocolSettings() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Any further default additions to the table will get rejected from now on.
  is_loaded_ = true;
  is_loading_ = true;

  Enable();

  RegisterProtocolHandlersFromPref(prefs::kRegisteredProtocolHandlers, USER);

  is_loading_ = false;

  // For each default protocol handler, check that we are still registered
  // with the OS as the default application.
  if (ShouldRemoveHandlersNotInOS()) {
    for (ProtocolHandlerMap::const_iterator p = default_handlers_.begin();
         p != default_handlers_.end(); ++p) {
      ProtocolHandler handler = p->second;
    }
  }
}

int ProtocolHandlerRegistry::GetHandlerIndex(const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const ProtocolHandler& handler = GetHandlerFor(scheme);
  if (handler.IsEmpty())
    return -1;
  const ProtocolHandlerList* handlers = GetHandlerList(scheme);
  if (!handlers)
    return -1;

  ProtocolHandlerList::const_iterator p;
  int i;
  for (i = 0, p = handlers->begin(); p != handlers->end(); ++p, ++i) {
    if (*p == handler)
      return i;
  }
  return -1;
}

ProtocolHandlerRegistry::ProtocolHandlerList
ProtocolHandlerRegistry::GetHandlersFor(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerMultiMap::const_iterator p = protocol_handlers_.find(scheme);
  if (p == protocol_handlers_.end()) {
    return ProtocolHandlerList();
  }
  return p->second;
}

void ProtocolHandlerRegistry::GetRegisteredProtocols(
    std::vector<std::string>* output) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerMultiMap::const_iterator p;
  for (p = protocol_handlers_.begin(); p != protocol_handlers_.end(); ++p) {
    if (!p->second.empty())
      output->push_back(p->first);
  }
}

bool ProtocolHandlerRegistry::CanSchemeBeOverridden(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const ProtocolHandlerList* handlers = GetHandlerList(scheme);
  // If we already have a handler for this scheme, we can add more.
  if (handlers != NULL && !handlers->empty())
    return true;
  // Don't override a scheme if it already has an external handler.
  return !delegate_->IsExternalHandlerRegistered(scheme);
}

bool ProtocolHandlerRegistry::IsRegistered(
    const ProtocolHandler& handler) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const ProtocolHandlerList* handlers = GetHandlerList(handler.protocol());
  if (!handlers) {
    return false;
  }
  return std::find(handlers->begin(), handlers->end(), handler) !=
      handlers->end();
}

bool ProtocolHandlerRegistry::IsRegisteredByUser(
    const ProtocolHandler& handler) {
  return HandlerExists(handler, &user_protocol_handlers_);
}

bool ProtocolHandlerRegistry::IsIgnored(const ProtocolHandler& handler) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerList::const_iterator i;
  for (i = ignored_protocol_handlers_.begin();
       i != ignored_protocol_handlers_.end(); ++i) {
    if (*i == handler) {
      return true;
    }
  }
  return false;
}

bool ProtocolHandlerRegistry::HasRegisteredEquivalent(
    const ProtocolHandler& handler) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const ProtocolHandlerList* handlers = GetHandlerList(handler.protocol());
  if (!handlers) {
    return false;
  }
  ProtocolHandlerList::const_iterator i;
  for (i = handlers->begin(); i != handlers->end(); ++i) {
    if (handler.IsEquivalent(*i)) {
      return true;
    }
  }
  return false;
}

bool ProtocolHandlerRegistry::HasIgnoredEquivalent(
    const ProtocolHandler& handler) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerList::const_iterator i;
  for (i = ignored_protocol_handlers_.begin();
       i != ignored_protocol_handlers_.end(); ++i) {
    if (handler.IsEquivalent(*i)) {
      return true;
    }
  }
  return false;
}

bool ProtocolHandlerRegistry::IsHandledProtocol(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return enabled_ && !GetHandlerFor(scheme).IsEmpty();
}

GURL ProtocolHandlerRegistry::TranslateUrl(
    const GURL& url) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandler handler = LookupHandler(default_handlers_,
                                          url.scheme());
  if (handler.IsEmpty())
    return url;

  GURL translated_url(handler.TranslateUrl(url));
  if (!translated_url.is_valid())
    return url;

  return translated_url;
}

void ProtocolHandlerRegistry::RemoveHandler(
    const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerList& handlers = protocol_handlers_[handler.protocol()];
  bool erase_success = false;
  if (HandlerExists(handler, handlers) &&
      HandlerExists(handler, &user_protocol_handlers_)) {
    EraseHandler(handler, &user_protocol_handlers_);
    erase_success = true;
    if (!HandlerExists(handler, &policy_protocol_handlers_))
      EraseHandler(handler, &protocol_handlers_);
  }
  ProtocolHandlerMap::iterator q = default_handlers_.find(handler.protocol());
  if (erase_success && q != default_handlers_.end() && q->second == handler) {
    // Make the new top handler in the list the default.
    if (!handlers.empty()) {
      // NOTE We pass a copy because SetDefault() modifies handlers.
      SetDefault(ProtocolHandler(handlers[0]));
    } else {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(&IOThreadDelegate::ClearDefault, io_thread_delegate_,
                     q->second.protocol()));

      default_handlers_.erase(q);
    }
  }

  if (erase_success && !IsHandledProtocol(handler.protocol())) {
    delegate_->DeregisterExternalHandler(handler.protocol());
  }
  Save();
  if (erase_success)
    NotifyChanged();
}

void ProtocolHandlerRegistry::RemoveDefaultHandler(const std::string& scheme) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandler current_default = GetHandlerFor(scheme);
  if (!current_default.IsEmpty())
    RemoveHandler(current_default);
}

const ProtocolHandler& ProtocolHandlerRegistry::GetHandlerFor(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return LookupHandler(default_handlers_, scheme);
}

void ProtocolHandlerRegistry::Enable() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (enabled_) {
    return;
  }
  enabled_ = true;
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&IOThreadDelegate::Enable, io_thread_delegate_));

  ProtocolHandlerMap::const_iterator p;
  for (p = default_handlers_.begin(); p != default_handlers_.end(); ++p) {
    delegate_->RegisterExternalHandler(p->first);
  }
  Save();
  NotifyChanged();
}

void ProtocolHandlerRegistry::Disable() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!enabled_) {
    return;
  }
  enabled_ = false;
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&IOThreadDelegate::Disable, io_thread_delegate_));

  ProtocolHandlerMap::const_iterator p;
  for (p = default_handlers_.begin(); p != default_handlers_.end(); ++p) {
    delegate_->DeregisterExternalHandler(p->first);
  }
  Save();
  NotifyChanged();
}

void ProtocolHandlerRegistry::Shutdown() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  delegate_.reset(NULL);

  weak_ptr_factory_.InvalidateWeakPtrs();
}

// static
void ProtocolHandlerRegistry::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(prefs::kRegisteredProtocolHandlers);
}

ProtocolHandlerRegistry::~ProtocolHandlerRegistry() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void ProtocolHandlerRegistry::PromoteHandler(const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(IsRegistered(handler));
  ProtocolHandlerMultiMap::iterator p =
      protocol_handlers_.find(handler.protocol());
  ProtocolHandlerList& list = p->second;
  list.erase(std::find(list.begin(), list.end(), handler));
  list.insert(list.begin(), handler);
}

void ProtocolHandlerRegistry::Save() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (is_loading_) {
    return;
  }
  scoped_ptr<base::Value> registered_protocol_handlers(
      EncodeRegisteredHandlers());
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);

  prefs->Set(prefs::kRegisteredProtocolHandlers,
      *registered_protocol_handlers);
}

const ProtocolHandlerRegistry::ProtocolHandlerList*
ProtocolHandlerRegistry::GetHandlerList(
    const std::string& scheme) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerMultiMap::const_iterator p = protocol_handlers_.find(scheme);
  if (p == protocol_handlers_.end()) {
    return NULL;
  }
  return &p->second;
}

void ProtocolHandlerRegistry::SetDefault(const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  default_handlers_.erase(handler.protocol());
  default_handlers_.insert(std::make_pair(handler.protocol(), handler));
  PromoteHandler(handler);
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&IOThreadDelegate::SetDefault, io_thread_delegate_, handler));
}

void ProtocolHandlerRegistry::InsertHandler(const ProtocolHandler& handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerMultiMap::iterator p =
      protocol_handlers_.find(handler.protocol());

  if (p != protocol_handlers_.end()) {
    p->second.push_back(handler);
    return;
  }

  ProtocolHandlerList new_list;
  new_list.push_back(handler);
  protocol_handlers_[handler.protocol()] = new_list;
}

base::Value* ProtocolHandlerRegistry::EncodeRegisteredHandlers() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::ListValue* protocol_handlers = new base::ListValue();
  for (ProtocolHandlerMultiMap::iterator i = user_protocol_handlers_.begin();
       i != user_protocol_handlers_.end();
       ++i) {
    for (ProtocolHandlerList::iterator j = i->second.begin();
         j != i->second.end(); ++j) {
      base::DictionaryValue* encoded = j->Encode();
      if (IsDefault(*j)) {
        encoded->Set("default", new base::FundamentalValue(true));
      }
      protocol_handlers->Append(encoded);
    }
  }
  return protocol_handlers;
}

void ProtocolHandlerRegistry::NotifyChanged() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROTOCOL_HANDLER_REGISTRY_CHANGED,
      content::Source<content::BrowserContext>(context_),
      content::NotificationService::NoDetails());
}

void ProtocolHandlerRegistry::RegisterProtocolHandler(
    const ProtocolHandler& handler,
    const HandlerSource source) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(CanSchemeBeOverridden(handler.protocol()));
  DCHECK(!handler.IsEmpty());
  ProtocolHandlerMultiMap& map =
      (source == POLICY) ? policy_protocol_handlers_ : user_protocol_handlers_;
  ProtocolHandlerList& list = map[handler.protocol()];
  if (!HandlerExists(handler, list))
    list.push_back(handler);
  if (IsRegistered(handler)) {
    return;
  }
  if (enabled_ && !delegate_->IsExternalHandlerRegistered(handler.protocol()))
    delegate_->RegisterExternalHandler(handler.protocol());
  InsertHandler(handler);
}

std::vector<const base::DictionaryValue*>
ProtocolHandlerRegistry::GetHandlersFromPref(const char* pref_name) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<const base::DictionaryValue*> result;
  PrefService* prefs = user_prefs::UserPrefs::Get(context_);
  if (!prefs->HasPrefPath(pref_name)) {
    return result;
  }

  const base::ListValue* handlers = prefs->GetList(pref_name);
  if (handlers) {
    for (size_t i = 0; i < handlers->GetSize(); ++i) {
      const base::DictionaryValue* dict;
      if (!handlers->GetDictionary(i, &dict))
        continue;
      if (ProtocolHandler::IsValidDict(dict)) {
        result.push_back(dict);
      }
    }
  }
  return result;
}

void ProtocolHandlerRegistry::RegisterProtocolHandlersFromPref(
    const char* pref_name,
    const HandlerSource source) {
  std::vector<const base::DictionaryValue*> registered_handlers =
      GetHandlersFromPref(pref_name);
  for (std::vector<const base::DictionaryValue*>::const_iterator p =
           registered_handlers.begin();
       p != registered_handlers.end();
       ++p) {
    ProtocolHandler handler = ProtocolHandler::CreateProtocolHandler(*p);
    RegisterProtocolHandler(handler, source);
    bool is_default = false;
    if ((*p)->GetBoolean("default", &is_default) && is_default) {
      SetDefault(handler);
    }
  }
}

void ProtocolHandlerRegistry::IgnoreProtocolHandler(
    const ProtocolHandler& handler,
    const HandlerSource source) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ProtocolHandlerList& list = (source == POLICY)
                                  ? policy_ignored_protocol_handlers_
                                  : user_ignored_protocol_handlers_;
  if (!HandlerExists(handler, list))
    list.push_back(handler);
  if (HandlerExists(handler, ignored_protocol_handlers_))
    return;
  ignored_protocol_handlers_.push_back(handler);
}

void ProtocolHandlerRegistry::IgnoreProtocolHandlersFromPref(
    const char* pref_name,
    const HandlerSource source) {
  std::vector<const base::DictionaryValue*> ignored_handlers =
      GetHandlersFromPref(pref_name);
  for (std::vector<const base::DictionaryValue*>::const_iterator p =
           ignored_handlers.begin();
       p != ignored_handlers.end();
       ++p) {
    IgnoreProtocolHandler(ProtocolHandler::CreateProtocolHandler(*p), source);
  }
}

bool ProtocolHandlerRegistry::HandlerExists(const ProtocolHandler& handler,
                                            ProtocolHandlerMultiMap* map) {
  return HandlerExists(handler, (*map)[handler.protocol()]);
}

bool ProtocolHandlerRegistry::HandlerExists(const ProtocolHandler& handler,
                                            const ProtocolHandlerList& list) {
  return std::find(list.begin(), list.end(), handler) != list.end();
}

void ProtocolHandlerRegistry::EraseHandler(const ProtocolHandler& handler,
                                           ProtocolHandlerMultiMap* map) {
  EraseHandler(handler, &(*map)[handler.protocol()]);
}

void ProtocolHandlerRegistry::EraseHandler(const ProtocolHandler& handler,
                                           ProtocolHandlerList* list) {
  list->erase(std::find(list->begin(), list->end(), handler));
}

void ProtocolHandlerRegistry::AddPredefinedHandler(
    const ProtocolHandler& handler) {
  DCHECK(!is_loaded_);  // Must be called prior InitProtocolSettings.
  RegisterProtocolHandler(handler, USER);
  SetDefault(handler);
}

scoped_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
ProtocolHandlerRegistry::CreateJobInterceptorFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // this is always created on the UI thread (in profile_io's
  // InitializeOnUIThread. Any method calls must be done
  // on the IO thread (this is checked).
  return scoped_ptr<JobInterceptorFactory>(
      new JobInterceptorFactory(io_thread_delegate_.get()));
}
