// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This interface is for managing the global services of the application. Each
// service is lazily created when requested the first time. The service getters
// will return NULL if the service is not available, so callers must check for
// this condition.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_H_
#define CHROME_BROWSER_BROWSER_PROCESS_H_

#include <memory>
#include <string>

#include "base/macros.h"

class IconManager;

namespace printing {
class PrintJobManager;
}

// NOT THREAD SAFE, call only from the main thread.
// These functions shouldn't return NULL unless otherwise noted.
class BrowserProcess {
 public:
  BrowserProcess();
  ~BrowserProcess();

  void SetApplicationLocale(const std::string& locale);
  std::string GetApplicationLocale();
  IconManager* GetIconManager();

  printing::PrintJobManager* print_job_manager();

 private:
  std::unique_ptr<printing::PrintJobManager> print_job_manager_;
  std::unique_ptr<IconManager> icon_manager_;
  std::string locale_;

  DISALLOW_COPY_AND_ASSIGN(BrowserProcess);
};

extern BrowserProcess* g_browser_process;

#endif  // CHROME_BROWSER_BROWSER_PROCESS_H_
