// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_WIDEVINE_CDM_CONSTANTS_H_
#define CHROME_COMMON_WIDEVINE_CDM_CONSTANTS_H_

#include "base/macros.h"
#include "base/files/file_path.h"

// The Widevine CDM adapter and Widevine CDM are in this directory.
extern const base::FilePath::CharType kWidevineCdmBaseDirectory[];

extern const char kWidevineCdmPluginExtension[];

// Permission bits for Widevine CDM plugin.
extern const int32_t kWidevineCdmPluginPermissions;

#endif  // CHROME_COMMON_WIDEVINE_CDM_CONSTANTS_H_
