// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_MONITOR_FINDER_MAC_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_MONITOR_FINDER_MAC_H_

#include <ApplicationServices/ApplicationServices.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"

namespace chrome {

// MonitorFinder maps a RenderFrameHost to the display ID on which the widget
// is painting. This class operates on the IO thread while the RenderFrameHost
// is on the UI thread, so the value returned by GetMonitor() may be 0 until
// the information can be retrieved asynchronously.
class MonitorFinder : public base::RefCountedThreadSafe<MonitorFinder> {
 public:
  MonitorFinder(int process_id, int render_frame_id);

  // Gets the native display ID for the <process_id, render_frame_id> tuple.
  int64_t GetMonitor();

  // Checks if the given |monitor_id| represents a built-in display.
  static bool IsMonitorBuiltIn(int64_t monitor_id);

 private:
  friend class base::RefCountedThreadSafe<MonitorFinder>;
  ~MonitorFinder();

  // Method run on the UI thread to get the display information.
  void FetchMonitorFromWidget();

  const int process_id_;
  const int render_frame_id_;

  base::Lock mutex_;  // Protects the two members below.
  // Whether one request to FetchMonitorFromWidget() has been made already.
  bool request_sent_;
  // The native display ID for the RenderFrameHost.
  CGDirectDisplayID display_id_;

  DISALLOW_COPY_AND_ASSIGN(MonitorFinder);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_MONITOR_FINDER_H_
