// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/renderer_host/atom_render_message_filter.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/predictors/loading_predictor.h"
#include "chrome/browser/predictors/loading_predictor_factory.h"
#include "chrome/browser/predictors/preconnect_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/network_hints/common/network_hints_common.h"
#include "components/network_hints/common/network_hints_messages.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"

using content::BrowserThread;

namespace {

const uint32_t kRenderFilteredMessageClasses[] = {
    NetworkHintsMsgStart,
};

}  // namespace

AtomRenderMessageFilter::AtomRenderMessageFilter(int render_process_id,
                                                 Profile* profile)
    : BrowserMessageFilter(kRenderFilteredMessageClasses,
                           base::size(kRenderFilteredMessageClasses)),
      render_process_id_(render_process_id),
      preconnect_manager_initialized_(false) {
  auto* loading_predictor =
      predictors::LoadingPredictorFactory::GetForProfile(profile);
  if (loading_predictor && loading_predictor->preconnect_manager()) {
    preconnect_manager_ = loading_predictor->preconnect_manager()->GetWeakPtr();
    preconnect_manager_->SetNetworkContextForTesting(
        content::BrowserContext::GetDefaultStoragePartition(profile)
            ->GetNetworkContext());
    preconnect_manager_initialized_ = true;
  }
}

AtomRenderMessageFilter::~AtomRenderMessageFilter() {}

bool AtomRenderMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomRenderMessageFilter, message)
    IPC_MESSAGE_HANDLER(NetworkHintsMsg_Preconnect, OnPreconnect)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AtomRenderMessageFilter::OnPreconnect(const GURL& url,
                                           bool allow_credentials,
                                           int count) {
  if (count < 1) {
    LOG(WARNING) << "NetworkHintsMsg_Preconnect IPC with invalid count: "
                 << count;
    return;
  }

  if (!url.is_valid() || !url.has_host() || !url.has_scheme() ||
      !url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  if (preconnect_manager_initialized_) {
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::UI},
        base::BindOnce(&predictors::PreconnectManager::StartPreconnectUrl,
                       preconnect_manager_, url, allow_credentials));
  }
}
