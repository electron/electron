// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ASAR_ASAR_UTIL_H_
#define ATOM_COMMON_ASAR_ASAR_UTIL_H_

#include <memory>
#include <string>

namespace base {
class FilePath;
}

namespace asar {

class Archive;

// Gets or creates a new Archive from the path.
std::shared_ptr<Archive> GetOrCreateAsarArchive(const base::FilePath& path);

// Separates the path to Archive out.
bool GetAsarArchivePath(const base::FilePath& full_path,
                        base::FilePath* asar_path,
                        base::FilePath* relative_path);

// Same with base::ReadFileToString but supports asar Archive.
bool ReadFileToString(const base::FilePath& path, std::string* contents);

}  // namespace asar

#endif  // ATOM_COMMON_ASAR_ASAR_UTIL_H_
