// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/pepper/monitor_finder_mac.h"

#import <Cocoa/Cocoa.h>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"

namespace chrome {

MonitorFinder::MonitorFinder(int process_id, int render_frame_id)
    : process_id_(process_id),
      render_frame_id_(render_frame_id),
      request_sent_(false),
      display_id_(kCGNullDirectDisplay) {}

MonitorFinder::~MonitorFinder() {}

int64_t MonitorFinder::GetMonitor() {
  {
    // The plugin may call this method several times, so avoid spamming the UI
    // thread with requests by only allowing one outstanding request at a time.
    base::AutoLock lock(mutex_);
    if (request_sent_)
      return display_id_;
    request_sent_ = true;
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&MonitorFinder::FetchMonitorFromWidget, this));
  return display_id_;
}

// static
bool MonitorFinder::IsMonitorBuiltIn(int64_t display_id) {
  return CGDisplayIsBuiltin(display_id);
}

void MonitorFinder::FetchMonitorFromWidget() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(process_id_, render_frame_id_);
  if (!rfh)
    return;

  gfx::NativeView native_view = rfh->GetNativeView();
  NSWindow* window = [native_view window];
  NSScreen* screen = [window screen];
  CGDirectDisplayID display_id =
      [[[screen deviceDescription] objectForKey:@"NSScreenNumber"] intValue];

  base::AutoLock lock(mutex_);
  request_sent_ = false;
  display_id_ = display_id;
}

}  // namespace chrome
