// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Electron: this is stubbed to remove a dependency
// on google_apis and components/cloud_devices

#ifndef COMPONENTS_CLOUD_DEVICES_COMMON_CLOUD_DEVICES_URLS_H_
#define COMPONENTS_CLOUD_DEVICES_COMMON_CLOUD_DEVICES_URLS_H_

#include <string>

#include "url/gurl.h"

namespace cloud_devices {

GURL GetCloudPrintRelativeURL(const std::string& relative_path);

}  // namespace cloud_devices

#endif  // COMPONENTS_CLOUD_DEVICES_COMMON_CLOUD_DEVICES_URLS_H_
