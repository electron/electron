// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/browser_client.h"

#include "base/lazy_instance.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "brightray/browser/browser_main_parts.h"
#include "brightray/browser/media/media_capture_devices_dispatcher.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"

using content::BrowserThread;

namespace brightray {

namespace {

BrowserClient* g_browser_client;

base::LazyInstance<std::string>::DestructorAtExit
    g_io_thread_application_locale = LAZY_INSTANCE_INITIALIZER;

base::NoDestructor<std::string> g_application_locale;

void SetApplicationLocaleOnIOThread(const std::string& locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  g_io_thread_application_locale.Get() = locale;
}

}  // namespace

// static
void BrowserClient::SetApplicationLocale(const std::string& locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!BrowserThread::IsThreadInitialized(BrowserThread::IO) ||
      !BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(&SetApplicationLocaleOnIOThread, locale))) {
    g_io_thread_application_locale.Get() = locale;
  }
  *g_application_locale = locale;
}

BrowserClient* BrowserClient::Get() {
  return g_browser_client;
}

BrowserClient::BrowserClient() : browser_main_parts_(nullptr) {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

BrowserClient::~BrowserClient() {}

BrowserMainParts* BrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new BrowserMainParts;
}

content::BrowserMainParts* BrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  DCHECK(!browser_main_parts_);
  browser_main_parts_ = OverrideCreateBrowserMainParts(parameters);
  return browser_main_parts_;
}

content::MediaObserver* BrowserClient::GetMediaObserver() {
  return MediaCaptureDevicesDispatcher::GetInstance();
}

base::FilePath BrowserClient::GetDefaultDownloadDirectory() {
  // ~/Downloads
  base::FilePath path;
  if (base::PathService::Get(base::DIR_HOME, &path))
    path = path.Append(FILE_PATH_LITERAL("Downloads"));

  return path;
}

std::string BrowserClient::GetApplicationLocale() {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO))
    return g_io_thread_application_locale.Get();
  return *g_application_locale;
}

}  // namespace brightray
