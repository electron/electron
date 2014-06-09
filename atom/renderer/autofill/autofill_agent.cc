// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/autofill/autofill_agent.h"

#include "atom/renderer/autofill/page_click_tracker.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace autofill {

AutofillAgent::AutofillAgent(content::RenderView* render_view)
    : content::RenderViewObserver(render_view) {
  render_view->GetWebView()->setAutofillClient(this);

  // The PageClickTracker is a RenderViewObserver, and hence will be freed when
  // the RenderView is destroyed.
  new PageClickTracker(render_view, this);
}

AutofillAgent::~AutofillAgent() {}

}  // namespace autofill
