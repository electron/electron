// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_APP_MANIFESTS_H_
#define SHELL_APP_MANIFESTS_H_

#include <vector>

#include "services/service_manager/public/cpp/manifest.h"

const service_manager::Manifest& GetElectronContentBrowserOverlayManifest();

#endif  // SHELL_APP_MANIFESTS_H_
