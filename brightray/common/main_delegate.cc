// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "common/main_delegate.h"

#include "browser/browser_client.h"
#include "common/content_client.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"

namespace brightray {

MainDelegate::MainDelegate() {
}

MainDelegate::~MainDelegate() {
}

scoped_ptr<ContentClient> MainDelegate::CreateContentClient() {
  return make_scoped_ptr(new ContentClient).Pass();
}

bool MainDelegate::BasicStartupComplete(int* exit_code) {
  content_client_ = CreateContentClient().Pass();
  SetContentClient(content_client_.get());
#if defined(OS_MACOSX)
  OverrideChildProcessPath();
  OverrideFrameworkBundlePath();
#endif
  return false;
}

void MainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
}

void MainDelegate::InitializeResourceBundle() {
  base::FilePath path;
#if defined(OS_MACOSX)
  path = GetResourcesPakFilePath();
#else
  base::FilePath pak_dir;
  PathService::Get(base::DIR_MODULE, &pak_dir);
  path = pak_dir.Append(FILE_PATH_LITERAL("content_shell.pak"));
#endif

  ui::ResourceBundle::InitSharedInstanceWithLocale("", NULL,
      ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(path, ui::SCALE_FACTOR_100P);
  AddDataPackFromPath(&ui::ResourceBundle::GetSharedInstance(), path.DirName());
}

content::ContentBrowserClient* MainDelegate::CreateContentBrowserClient() {
  browser_client_ = CreateBrowserClient().Pass();
  return browser_client_.get();
}

scoped_ptr<BrowserClient> MainDelegate::CreateBrowserClient() {
  return make_scoped_ptr(new BrowserClient).Pass();
}

}  // namespace brightray
