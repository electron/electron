// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_RENDERER_HOST_ATOM_RENDER_MESSAGE_FILTER_H_
#define ATOM_BROWSER_RENDERER_HOST_ATOM_RENDER_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "ppapi/buildflags/buildflags.h"

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
  AtomRenderMessageFilter(int render_process_id, Profile* profile);

  // content::BrowserMessageFilter methods:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<AtomRenderMessageFilter>;

  ~AtomRenderMessageFilter() override;

  void OnPreconnect(const GURL& url, bool allow_credentials, int count);

  const int render_process_id_;

  base::WeakPtrFactory<AtomPreconnectDelegate> weak_factory_;

  // The PreconnectManager for the associated Profile. This must only be
  // accessed on the UI thread.
  static predictors::PreconnectManager* preconnect_manager_;

  // Allows to check on the IO thread whether the PreconnectManager was
  // initialized.
  bool preconnect_manager_initialized_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderMessageFilter);
};

#endif  // ATOM_BROWSER_RENDERER_HOST_ATOM_RENDER_MESSAGE_FILTER_H_
