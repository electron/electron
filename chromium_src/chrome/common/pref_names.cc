// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/pref_names.h"

namespace prefs {

const char kSelectFileLastDirectory[] = "selectfile.last_directory";
const char kDownloadDefaultDirectory[] = "download.default_directory";
const char kDevToolsFileSystemPaths[] = "devtools.file_system_paths";
const char kApplicationLocale[] = "intl.app_locale";

// List of protocol handlers.
const char kRegisteredProtocolHandlers[] =
  "custom_handlers.registered_protocol_handlers";

// List of protocol handlers the user has requested not to be asked about again.
const char kIgnoredProtocolHandlers[] =
  "custom_handlers.ignored_protocol_handlers";

// List of protocol handlers registered by policy.
const char kPolicyRegisteredProtocolHandlers[] =
    "custom_handlers.policy.registered_protocol_handlers";

// List of protocol handlers the policy has requested to be ignored.
const char kPolicyIgnoredProtocolHandlers[] =
    "custom_handlers.policy.ignored_protocol_handlers";

// Whether user-specified handlers for protocols and content types can be
// specified.
const char kCustomHandlersEnabled[] = "custom_handlers.enabled";

// Double that indicates the default zoom level.
const char kPartitionDefaultZoomLevel[] = "partition.default_zoom_level";

// Dictionary that maps hostnames to zoom levels.  Hosts not in this pref will
// be displayed at the default zoom level.
const char kPartitionPerHostZoomLevels[] = "partition.per_host_zoom_levels";

}  // namespace prefs
