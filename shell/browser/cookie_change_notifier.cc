// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/cookie_change_notifier.h"

#include "base/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/cookies/canonical_cookie.h"
#include "shell/browser/electron_browser_context.h"

using content::BrowserThread;

namespace electron {

CookieChangeNotifier::CookieChangeNotifier(
    ElectronBrowserContext* browser_context)
    : browser_context_(browser_context), receiver_(this) {
  StartListening();
}

CookieChangeNotifier::~CookieChangeNotifier() = default;

base::CallbackListSubscription
CookieChangeNotifier::RegisterCookieChangeCallback(
    const base::RepeatingCallback<void(const net::CookieChangeInfo& change)>&
        cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  return cookie_change_sub_list_.Add(cb);
}

void CookieChangeNotifier::StartListening() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!receiver_.is_bound());

  network::mojom::CookieManager* cookie_manager =
      browser_context_->GetDefaultStoragePartition()
          ->GetCookieManagerForBrowserProcess();

  // Cookie manager should be created whenever network context is created,
  // if this fails then there is something wrong with our context creation
  // cycle.
  CHECK(cookie_manager);

  cookie_manager->AddGlobalChangeListener(receiver_.BindNewPipeAndPassRemote());
  receiver_.set_disconnect_handler(base::BindOnce(
      &CookieChangeNotifier::OnConnectionError, base::Unretained(this)));
}

void CookieChangeNotifier::OnConnectionError() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  receiver_.reset();
  StartListening();
}

void CookieChangeNotifier::OnCookieChange(const net::CookieChangeInfo& change) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  cookie_change_sub_list_.Notify(change);
}

}  // namespace electron
