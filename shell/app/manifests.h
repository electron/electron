// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_APP_MANIFESTS_H_
#define SHELL_APP_MANIFESTS_H_

#include <vector>

#include "electron/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/manifest.h"

const service_manager::Manifest& GetElectronContentBrowserOverlayManifest();

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
const service_manager::Manifest& GetElectronContentRendererOverlayManifest();
#endif

#endif  // SHELL_APP_MANIFESTS_H_
