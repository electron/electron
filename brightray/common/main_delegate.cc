// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "common/main_delegate.h"

#include <memory>

#include "browser/browser_client.h"
#include "common/content_client.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"

namespace brightray {

namespace {

// Returns true if this subprocess type needs the ResourceBundle initialized
// and resources loaded.
bool SubprocessNeedsResourceBundle(const std::string& process_type) {
  return
#if defined(OS_POSIX) && !defined(OS_MACOSX)
      // The zygote process opens the resources for the renderers.
      process_type == switches::kZygoteProcess ||
#endif
#if defined(OS_MACOSX)
      // Mac needs them too for scrollbar related images and for sandbox
      // profiles.
#if !defined(DISABLE_NACL)
      process_type == switches::kNaClLoaderProcess ||
#endif
      process_type == switches::kPpapiPluginProcess ||
      process_type == switches::kPpapiBrokerProcess ||
      process_type == switches::kGpuProcess ||
#endif
      process_type == switches::kRendererProcess ||
      process_type == switches::kUtilityProcess;
}

}  // namespace

void InitializeResourceBundle(const std::string& locale) {
  // Load locales.
  ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, nullptr, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);

  // Load other resource files.
  base::FilePath path;
#if defined(OS_MACOSX)
  path = GetResourcesPakFilePath();
#else
  base::FilePath pak_dir;
  PathService::Get(base::DIR_MODULE, &pak_dir);
  path = pak_dir.Append(FILE_PATH_LITERAL("content_shell.pak"));
#endif

  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  bundle.AddDataPackFromPath(path, ui::GetSupportedScaleFactors()[0]);
#if !defined(OS_MACOSX)
  bundle.AddDataPackFromPath(
      pak_dir.Append(FILE_PATH_LITERAL("blink_image_resources_200_percent.pak")),
      ui::SCALE_FACTOR_200P);
  bundle.AddDataPackFromPath(
      pak_dir.Append(FILE_PATH_LITERAL("content_resources_200_percent.pak")),
      ui::SCALE_FACTOR_200P);
  bundle.AddDataPackFromPath(
      pak_dir.Append(FILE_PATH_LITERAL("ui_resources_200_percent.pak")),
      ui::SCALE_FACTOR_200P);
  bundle.AddDataPackFromPath(
      pak_dir.Append(FILE_PATH_LITERAL("views_resources_200_percent.pak")),
      ui::SCALE_FACTOR_200P);
#endif
}


MainDelegate::MainDelegate() {
}

MainDelegate::~MainDelegate() {
}

std::unique_ptr<ContentClient> MainDelegate::CreateContentClient() {
  return std::unique_ptr<ContentClient>(new ContentClient);
}

bool MainDelegate::BasicStartupComplete(int* exit_code) {
  content_client_ = CreateContentClient();
  SetContentClient(content_client_.get());
#if defined(OS_MACOSX)
  OverrideChildProcessPath();
  OverrideFrameworkBundlePath();
#endif
  return false;
}

void MainDelegate::PreSandboxStartup() {
  auto cmd = *base::CommandLine::ForCurrentProcess();
  std::string process_type = cmd.GetSwitchValueASCII(switches::kProcessType);

  // Initialize ResourceBundle which handles files loaded from external
  // sources. The language should have been passed in to us from the
  // browser process as a command line flag.
  if (SubprocessNeedsResourceBundle(process_type)) {
    std::string locale = cmd.GetSwitchValueASCII(switches::kLang);
    InitializeResourceBundle(locale);
  }
}

content::ContentBrowserClient* MainDelegate::CreateContentBrowserClient() {
  browser_client_ = CreateBrowserClient();
  return browser_client_.get();
}

std::unique_ptr<BrowserClient> MainDelegate::CreateBrowserClient() {
  return std::unique_ptr<BrowserClient>(new BrowserClient);
}

}  // namespace brightray
