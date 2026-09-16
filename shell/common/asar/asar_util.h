// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ASAR_ASAR_UTIL_H_
#define ELECTRON_SHELL_COMMON_ASAR_ASAR_UTIL_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/containers/span.h"

namespace base {
class FilePath;
}

class GURL;

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

// The fs wrapper's per-call question, answered without building FilePaths:
// how many leading bytes of the UTF-8 |path| name an asar archive file (the
// deepest "*.asar" component that is not a directory on disk), so that the
// rest is the entry inside it. Returns kNotInArchive when no component
// qualifies. When |require_normalized| is set and the path would be inside an
// archive but has empty, "." or ".." components, returns kNeedsNormalization
// instead so the caller can normalize lexically and ask again.
inline constexpr int kNotInArchive = -1;
inline constexpr int kNeedsNormalization = -2;
int FindArchivePrefixLength(std::string_view path, bool require_normalized);

// Same with base::ReadFileToString but supports asar Archive.
bool ReadFileToString(const base::FilePath& path, std::string* contents);

// For a file: URL inside an asar archive, returns a file: URL to an
// extracted copy of the entry plus the entry's name; false otherwise.
bool GetExtractedFileURL(const GURL& url,
                         GURL* extracted_url,
                         std::u16string* file_name);

void ValidateIntegrityOrDie(base::span<const uint8_t> input,
                            const IntegrityPayload& integrity,
                            std::string_view what = {});

}  // namespace asar

#endif  // ELECTRON_SHELL_COMMON_ASAR_ASAR_UTIL_H_
