// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "shell/app/electron_library_main.h"

#include <dlfcn.h>

#include "base/apple/bundle_locations.h"
#include "base/apple/scoped_nsautorelease_pool.h"
#include "base/at_exit.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/notreached.h"
#include "content/public/app/content_main.h"
#include "electron/fuses.h"
#include "electron/mas.h"
#include "shell/app/electron_main_delegate.h"
#include "shell/app/node_main.h"
#include "shell/app_shim/app_shim_main_delegate.h"
#include "shell/common/app_shim_constants.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/mac/main_application_bundle.h"
#include "shell/common/uv_includes.h"

namespace {

#if !IS_MAS_BUILD()
// The .app that holds the framework this code was loaded from: the app shim's
// host. Empty when the framework is not inside an app bundle.
base::FilePath HostBundlePathFromLoadedFramework() {
  Dl_info info;
  if (!dladdr(reinterpret_cast<const void*>(&ElectronAppShimMain), &info) ||
      !info.dli_fname) {
    return {};
  }
  base::FilePath path =
      base::MakeAbsoluteFilePath(base::FilePath(info.dli_fname));
  // <host>.app/Contents/Frameworks/<name>.framework/Versions/A/<name>
  while (!path.empty() && path != path.DirName() &&
         path.Extension() != ".framework") {
    path = path.DirName();
  }
  if (path.Extension() != ".framework")
    return {};
  path = path.DirName().DirName().DirName();
  return path.Extension() == ".app" ? path : base::FilePath();
}
#endif  // !IS_MAS_BUILD()

}  // namespace

int ElectronMain(int argc, char* argv[]) {
  argv = uv_setup_args(argc, argv);
  base::CommandLine::Init(argc, argv);
  electron::ElectronCommandLine::Init(argc, argv);

  electron::ElectronMainDelegate delegate;

  // Ensure that Bundle Id is set before ContentMain.
  // Refs https://chromium-review.googlesource.com/c/chromium/src/+/5581006
  delegate.OverrideChildProcessPath();
  delegate.OverrideFrameworkBundlePath();
  delegate.SetUpBundleOverrides();

  content::ContentMainParams params{&delegate};
  params.argc = argc;
  params.argv = UNSAFE_BUFFERS(const_cast<const char**>(argv));
  return content::ContentMain(std::move(params));
}

int ElectronInitializeICUandStartNode(int argc, char* argv[]) {
  if (!electron::fuses::IsRunAsNodeEnabled()) {
    NOTREACHED() << "run_as_node fuse is disabled";
  }

  argv = uv_setup_args(argc, argv);
  base::CommandLine::Init(argc, argv);
  electron::ElectronCommandLine::Init(argc, argv);

  base::AtExitManager atexit_manager;
  base::apple::ScopedNSAutoreleasePool pool;
  base::apple::SetOverrideFrameworkBundlePath(
      electron::MainApplicationBundlePath()
          .Append("Contents")
          .Append("Frameworks")
          .Append(ELECTRON_PRODUCT_NAME " Framework.framework"));
  base::i18n::InitializeICU();
  return electron::NodeMain();
}

#if !IS_MAS_BUILD()
int ElectronAppShimMain(int argc, char* argv[]) {
  const base::FilePath host_bundle_path = HostBundlePathFromLoadedFramework();
  CHECK(!host_bundle_path.empty())
      << "An app shim must load the framework from inside its host app";

  // The main bundle is the shim's; resources come from the host's.
  base::apple::SetOverrideOuterBundlePath(host_bundle_path);
  base::apple::SetOverrideFrameworkBundlePath(
      host_bundle_path.Append("Contents")
          .Append("Frameworks")
          .Append(ELECTRON_PRODUCT_NAME " Framework.framework"));

  base::CommandLine::Init(argc, argv);
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      "type", electron::app_shim::kProcessType);

  electron::AppShimMainDelegate delegate(host_bundle_path);
  content::ContentMainParams params{&delegate};
  return content::ContentMain(std::move(params));
}
#endif  // !IS_MAS_BUILD()
