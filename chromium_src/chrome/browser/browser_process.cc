// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "chrome/browser/printing/print_job_manager.h"
#endif
#include "printing/buildflags/buildflags.h"
#include "ui/base/l10n/l10n_util.h"

BrowserProcess* g_browser_process = NULL;

BrowserProcess::BrowserProcess() : print_job_manager_(nullptr) {
  g_browser_process = this;
}

BrowserProcess::~BrowserProcess() {
  g_browser_process = NULL;
}

void BrowserProcess::SetApplicationLocale(const std::string& locale) {
  locale_ = locale;
}

std::string BrowserProcess::GetApplicationLocale() {
  return locale_;
}

printing::PrintJobManager* BrowserProcess::print_job_manager() {
#if BUILDFLAG(ENABLE_PRINTING)
  if (!print_job_manager_)
    print_job_manager_.reset(new printing::PrintJobManager());
  return print_job_manager_.get();
#else
  return nullptr;
#endif
}
