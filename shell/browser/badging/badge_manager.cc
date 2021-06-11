// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/badging/badge_manager.h"

#include <utility>

#include "base/i18n/number_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/badging/badge_manager_factory.h"
#include "shell/browser/browser.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

namespace badging {

BadgeManager::BadgeManager() = default;
BadgeManager::~BadgeManager() = default;

// static
void BadgeManager::BindFrameReceiver(
    content::RenderFrameHost* frame,
    mojo::PendingReceiver<blink::mojom::BadgeService> receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* browser_context =
      content::WebContents::FromRenderFrameHost(frame)->GetBrowserContext();

  auto* badge_manager =
      badging::BadgeManagerFactory::GetInstance()->GetForBrowserContext(
          browser_context);
  if (!badge_manager)
    return;

  auto context = std::make_unique<FrameBindingContext>(
      frame->GetProcess()->GetID(), frame->GetRoutingID());

  badge_manager->receivers_.Add(badge_manager, std::move(receiver),
                                std::move(context));
}

void BadgeManager::BindServiceWorkerReceiver(
    content::RenderProcessHost* service_worker_process_host,
    const GURL& service_worker_scope,
    mojo::PendingReceiver<blink::mojom::BadgeService> receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* browser_context = service_worker_process_host->GetBrowserContext();

  auto* badge_manager =
      badging::BadgeManagerFactory::GetInstance()->GetForBrowserContext(
          browser_context);
  if (!badge_manager)
    return;

  auto context = std::make_unique<BadgeManager::ServiceWorkerBindingContext>(
      service_worker_process_host->GetID(), service_worker_scope);

  badge_manager->receivers_.Add(badge_manager, std::move(receiver),
                                std::move(context));
}

std::string BadgeManager::GetBadgeString(absl::optional<int> badge_content) {
  if (!badge_content)
    return "•";

  if (badge_content > kMaxBadgeContent) {
    return base::UTF16ToUTF8(l10n_util::GetStringFUTF16(
        IDS_SATURATED_BADGE_CONTENT, base::FormatNumber(kMaxBadgeContent)));
  }

  return base::UTF16ToUTF8(base::FormatNumber(badge_content.value()));
}

void BadgeManager::SetBadge(blink::mojom::BadgeValuePtr mojo_value) {
  if (mojo_value->is_number() && mojo_value->get_number() == 0) {
    mojo::ReportBadMessage(
        "|value| should not be zero when it is |number| (ClearBadge should be "
        "called instead)!");
    return;
  }

  absl::optional<int> value =
      mojo_value->is_flag() ? absl::nullopt
                            : absl::make_optional(mojo_value->get_number());

  electron::Browser::Get()->SetBadgeCount(value);
}

void BadgeManager::ClearBadge() {
  electron::Browser::Get()->SetBadgeCount(0);
}

}  // namespace badging
