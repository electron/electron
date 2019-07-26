// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/net/preconnect_manager_factory.h"
#include "shell/browser/net/preconnect_manager_helper.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/native_mate_converters/gurl_converter.h"
#include "shell/common/options_switches.h"

namespace electron {

PreconnectManagerHelper::PreconnectManagerHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), session_(nullptr) {}

PreconnectManagerHelper::~PreconnectManagerHelper() = default;

void PreconnectManagerHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!session_)
    return;

  if (navigation_handle->GetURL().is_empty() ||
      !navigation_handle->GetURL().SchemeIsHTTPOrHTTPS())
    return;

  session_->Emit("preconnect", navigation_handle->GetURL(), true);
}

void PreconnectManagerHelper::SetSession(api::Session* session) {
  session_ = session;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PreconnectManagerHelper)

}  // namespace electron
