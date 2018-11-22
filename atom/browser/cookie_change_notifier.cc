// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/cookie_change_notifier.h"

#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/cookie_details.h"
#include "base/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/cookies/canonical_cookie.h"

using content::BrowserThread;

namespace atom {

CookieChangeNotifier::CookieChangeNotifier(AtomBrowserContext* browser_context)
    : browser_context_(browser_context), binding_(this) {
  StartListening();
}

CookieChangeNotifier::~CookieChangeNotifier() = default;

std::unique_ptr<base::CallbackList<void(const CookieDetails*)>::Subscription>
CookieChangeNotifier::RegisterCookieChangeCallback(
    const base::Callback<void(const CookieDetails*)>& cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return cookie_change_sub_list_.Add(cb);
}

void CookieChangeNotifier::StartListening() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!binding_.is_bound());

  network::mojom::CookieManager* cookie_manager =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_)
          ->GetCookieManagerForBrowserProcess();
  // Cookie manager should be created whenever network context is created,
  // if this fails then there is something wrong with our context creation
  // cycle.
  CHECK(cookie_manager);

  network::mojom::CookieChangeListenerPtr listener_ptr;
  binding_.Bind(mojo::MakeRequest(&listener_ptr));
  binding_.set_connection_error_handler(base::BindOnce(
      &CookieChangeNotifier::OnConnectionError, base::Unretained(this)));

  cookie_manager->AddGlobalChangeListener(std::move(listener_ptr));
}

void CookieChangeNotifier::OnConnectionError() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  binding_.Close();
  StartListening();
}

void CookieChangeNotifier::OnCookieChange(
    const net::CanonicalCookie& cookie,
    network::mojom::CookieChangeCause cause) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  CookieDetails cookie_details(
      &cookie, cause != network::mojom::CookieChangeCause::INSERTED, cause);
  cookie_change_sub_list_.Notify(&cookie_details);
}

}  // namespace atom
