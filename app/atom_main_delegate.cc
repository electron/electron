// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/atom_main_delegate.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "browser/atom_browser_client.h"
#include "content/public/common/content_switches.h"
#include "renderer/atom_renderer_client.h"

#if defined(OS_MACOSX)

#include "base/mac/bundle_locations.h"
#include "base/path_service.h"
#include "content/public/common/content_paths.h"

namespace brightray {
base::FilePath MainApplicationBundlePath();
}

namespace {

base::FilePath GetFrameworksPath() {
  return brightray::MainApplicationBundlePath().Append("Contents")
                                               .Append("Frameworks");
}

}  // namespace

#endif  // defined(OS_MACOSX)

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
  brightray::MainDelegate::PreSandboxStartup();

#if defined(OS_MACOSX)
  // Override the path to helper process, since third party users may want to
  // change the application name.
  base::FilePath helper_path = GetFrameworksPath().Append("Atom Helper.app")
                                                  .Append("Contents")
                                                  .Append("MacOS")
                                                  .Append("Atom Helper");
  PathService::Override(content::CHILD_PROCESS_EXE, helper_path);
#endif  // defined(OS_MACOSX)

  // Disable renderer sandbox for most of node's functions.
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kNoSandbox);
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
