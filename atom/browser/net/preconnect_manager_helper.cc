// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/net/preconnect_manager_factory.h"
#include "atom/browser/net/preconnect_manager_tab_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/options_switches.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

namespace atom {

PreconnectManagerHelper::PreconnectManagerHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      preconnect_manager_(nullptr),
      number_of_sockets_to_preconnect_(-1) {
  preconnect_manager_ = atom::PreconnectManagerFactory::GetForContext(
      web_contents->GetBrowserContext());
}

PreconnectManagerHelper::~PreconnectManagerHelper() = default;

void PreconnectManagerHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!preconnect_manager_)
    return;

  if (navigation_handle->GetURL().is_empty() ||
      !navigation_handle->GetURL().SchemeIsHTTPOrHTTPS())
    return;

  if (number_of_sockets_to_preconnect_ >= 0) {
    std::vector<predictors::PreconnectRequest> requests;
    requests.emplace_back(navigation_handle->GetURL().GetOrigin(),
                          number_of_sockets_to_preconnect_);

    base::PostTaskWithTraits(
        FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(&predictors::PreconnectManager::Start,
                       base::Unretained(preconnect_manager_),
                       navigation_handle->GetURL(), requests));
  }
}

static bool GetAsInteger(const base::Value* val,
                         const base::StringPiece& path,
                         int* out) {
  if (val) {
    auto* found = val->FindKey(path);
    if (found && found->is_int()) {
      *out = found->GetInt();
      return true;
    } else if (found && found->is_string()) {
      return base::StringToInt(found->GetString(), out);
    }
  }
  return false;
}

int PreconnectManagerHelper::GetNumberOfSocketsToPreconnect(
    WebContentsPreferences* prefs) {
  int num_sockets_to_preconnect = -1;
  if (GetAsInteger(prefs->preference(), options::kNumSocketsToPreconnect,
                   &num_sockets_to_preconnect)) {
    const int kMinSocketsToPreconnect = 1;
    const int kMaxSocketsToPreconnect = 6;
    num_sockets_to_preconnect =
        std::max(num_sockets_to_preconnect, kMinSocketsToPreconnect);
    num_sockets_to_preconnect =
        std::min(num_sockets_to_preconnect, kMaxSocketsToPreconnect);
  }
  return num_sockets_to_preconnect;
}

void PreconnectManagerHelper::SetNumberOfSocketsToPreconnect(int numSockets) {
  number_of_sockets_to_preconnect_ = numSockets;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PreconnectManagerHelper)

}  // namespace atom
