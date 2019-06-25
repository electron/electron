// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/renderer_host/atom_render_message_filter.h"

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
#include "chrome/browser/predictors/preconnect_manager.h"
#include "components/network_hints/common/network_hints_common.h"
#include "components/network_hints/common/network_hints_messages.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "shell/browser/net/preconnect_manager_factory.h"

using content::BrowserThread;

namespace {

const uint32_t kRenderFilteredMessageClasses[] = {
    NetworkHintsMsgStart,
};

}  // namespace

AtomRenderMessageFilter::AtomRenderMessageFilter(
    int render_process_id,
    content::BrowserContext* context,
    int number_of_sockets_to_preconnect)
    : BrowserMessageFilter(kRenderFilteredMessageClasses,
                           base::size(kRenderFilteredMessageClasses)),
      preconnect_manager_(nullptr),
      number_of_sockets_to_preconnect_(number_of_sockets_to_preconnect) {
  preconnect_manager_ =
      electron::PreconnectManagerFactory::GetForContext(context);
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

  if (!preconnect_manager_) {
    return;
  }

  if (number_of_sockets_to_preconnect_ > 0) {
    std::vector<predictors::PreconnectRequest> requests;
    requests.emplace_back(url.GetOrigin(), number_of_sockets_to_preconnect_);

    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::UI},
        base::BindOnce(&predictors::PreconnectManager::Start,
                       base::Unretained(preconnect_manager_), url, requests));
  }
}

namespace predictors {

PreconnectRequest::PreconnectRequest(const GURL& origin, int num_sockets)
    : origin(origin), num_sockets(num_sockets) {
  DCHECK_GE(num_sockets, 0);
}

}  // namespace predictors
