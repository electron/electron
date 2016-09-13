// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/widevine_cdm_constants.h"

#include "build/build_config.h"
#include "ppapi/shared_impl/ppapi_permissions.h"

const base::FilePath::CharType kWidevineCdmBaseDirectory[] =
    FILE_PATH_LITERAL("WidevineCDM");

const char kWidevineCdmPluginExtension[] = "";

const int32_t kWidevineCdmPluginPermissions = ppapi::PERMISSION_DEV |
                                              ppapi::PERMISSION_PRIVATE;
