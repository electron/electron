// Copyright (c) 2025 Noah Gregory
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ASAR_INTEGRITY_DIGEST_H_
#define ELECTRON_SHELL_COMMON_ASAR_INTEGRITY_DIGEST_H_

#include <Foundation/Foundation.h>
namespace asar {

bool IsIntegrityDictionaryValid(NSDictionary* integrity_dict);

}  // namespace asar

#endif  // ELECTRON_SHELL_COMMON_ASAR_INTEGRITY_DIGEST_H_
