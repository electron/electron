// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ASAR_ASAR_UTIL_H_
#define ELECTRON_SHELL_COMMON_ASAR_ASAR_UTIL_H_

#include <memory>
#include <string>

#include "base/containers/span.h"

namespace base {
class FilePath;
}

namespace asar {

class Archive;
struct IntegrityPayload;

// Gets or creates and caches a new Archive from the path.
std::shared_ptr<Archive> GetOrCreateAsarArchive(const base::FilePath& path);

// Separates the path to Archive out.
bool GetAsarArchivePath(const base::FilePath& full_path,
                        base::FilePath* asar_path,
                        base::FilePath* relative_path,
                        bool allow_root = false);

// Same with base::ReadFileToString but supports asar Archive.
bool ReadFileToString(const base::FilePath& path, std::string* contents);

void ValidateIntegrityOrDie(base::span<const uint8_t> input,
                            const IntegrityPayload& integrity);

}  // namespace asar

#endif  // ELECTRON_SHELL_COMMON_ASAR_ASAR_UTIL_H_
