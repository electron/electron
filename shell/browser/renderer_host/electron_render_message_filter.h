// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_RENDERER_HOST_ELECTRON_RENDER_MESSAGE_FILTER_H_
#define SHELL_BROWSER_RENDERER_HOST_ELECTRON_RENDER_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "content/public/browser/browser_message_filter.h"
#include "shell/browser/api/atom_api_web_contents.h"

class GURL;

namespace content {
class BrowserContext;
class WebCursor;
}  // namespace content

namespace predictors {
class PreconnectManager;
}

// This class filters out incoming Chrome-specific IPC messages for the renderer
// process on the IPC thread.
class ElectronRenderMessageFilter : public content::BrowserMessageFilter {
 public:
  ElectronRenderMessageFilter(
      content::BrowserContext* browser_context,
      mate::Handle<electron::api::WebContents> web_contents);

  // content::BrowserMessageFilter methods:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~ElectronRenderMessageFilter() override;

  void OnPreconnect(int render_frame_id,
                    const GURL& url,
                    bool allow_credentials,
                    int count);
  void OnCursorChange(const content::WebCursor& cursor);

  content::BrowserContext* browser_context_;
  mate::Handle<electron::api::WebContents> web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRenderMessageFilter);
};

#endif  // SHELL_BROWSER_RENDERER_HOST_ELECTRON_RENDER_MESSAGE_FILTER_H_
