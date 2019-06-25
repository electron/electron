// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_RENDERER_HOST_ATOM_RENDER_MESSAGE_FILTER_H_
#define SHELL_BROWSER_RENDERER_HOST_ATOM_RENDER_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_message_filter.h"

class GURL;
class Profile;

namespace predictors {
class PreconnectManager;
}

namespace content_settings {
class CookieSettings;
}

namespace network_hints {
struct LookupRequest;
}

class AtomPreconnectDelegate;

// This class filters out incoming Chrome-specific IPC messages for the renderer
// process on the IPC thread.
class AtomRenderMessageFilter : public content::BrowserMessageFilter {
 public:
  AtomRenderMessageFilter(int render_process_id,
                          content::BrowserContext* context,
                          int number_of_sockets_to_preconnect);

  // content::BrowserMessageFilter methods:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~AtomRenderMessageFilter() override;

  void OnPreconnect(const GURL& url, bool allow_credentials, int count);

  // The PreconnectManager for the associated context. This must only be
  // accessed on the UI thread.
  predictors::PreconnectManager* preconnect_manager_;

  int number_of_sockets_to_preconnect_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderMessageFilter);
};

#endif  // SHELL_BROWSER_RENDERER_HOST_ATOM_RENDER_MESSAGE_FILTER_H_
