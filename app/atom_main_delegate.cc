// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/atom_main_delegate.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "browser/atom_browser_client.h"
#include "content/public/common/content_switches.h"
#include "renderer/atom_renderer_client.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/path_service.h"

namespace atom {

AtomMainDelegate::AtomMainDelegate() {
}

AtomMainDelegate::~AtomMainDelegate() {
}

bool AtomMainDelegate::BasicStartupComplete(int* exit_code) {
  // Disable logging out to debug.log on Windows
#if defined(OS_WIN)
  logging::InitLogging(
      L"debug.log",
#if defined(DEBUG)
      logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG ,
#else
      logging::LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
#endif  // defined(NDEBUG)
      logging::LOCK_LOG_FILE,
      logging::DELETE_OLD_LOG_FILE,
      logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);
  logging::SetLogItems(true, false, true, false);
#endif  // defined(OS_WIN)

  return brightray::MainDelegate::BasicStartupComplete(exit_code);
}

void AtomMainDelegate::PreSandboxStartup() {
#if defined(OS_MACOSX)
  OverrideChildProcessPath();
  OverrideFrameworkBundlePath();
#endif
  InitializeResourceBundle();

  CommandLine* command_line = CommandLine::ForCurrentProcess();

  // Disable renderer sandbox for most of node's functions.
  command_line->AppendSwitch(switches::kNoSandbox);

  // Disable accelerated compositing since it caused a lot of troubles (black
  // devtools, screen flashes) and needed lots of effort to make it right.
  command_line->AppendSwitch(switches::kDisableAcceleratedCompositing);

  // Add a flag to mark the end of switches added by atom-shell.
  command_line->AppendSwitch("no-more-switches");
}

void AtomMainDelegate::InitializeResourceBundle() {
  base::FilePath path;
#if defined(OS_MACOSX)
  path = GetResourcesPakFilePath();
#else
  base::FilePath pak_dir;
  PathService::Get(base::DIR_MODULE, &pak_dir);
  path = pak_dir.Append(FILE_PATH_LITERAL("content_shell.pak"));
#endif

  ui::ResourceBundle::InitSharedInstanceWithPakPath(path);
}

content::ContentBrowserClient* AtomMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new AtomBrowserClient);
  return browser_client_.get();
}

content::ContentRendererClient*
    AtomMainDelegate::CreateContentRendererClient() {
  renderer_client_.reset(new AtomRendererClient);
  return renderer_client_.get();
}

}  // namespace atom
