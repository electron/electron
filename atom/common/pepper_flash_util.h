// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PEPPER_FLASH_UTIL_H_
#define ATOM_COMMON_PEPPER_FLASH_UTIL_H_

#include <string>
#include <vector>
#include "base/files/file_path.h"
#include "content/public/common/pepper_plugin_info.h"

namespace atom {

content::PepperPluginInfo CreatePepperFlashInfo(const base::FilePath& path,
                                                const std::string& version);

void AddPepperFlashFromCommandLine(
    std::vector<content::PepperPluginInfo>* plugins);

}  // namespace atom

#endif  // ATOM_COMMON_PEPPER_FLASH_UTIL_H_
