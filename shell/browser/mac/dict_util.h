// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MAC_DICT_UTIL_H_
#define ELECTRON_SHELL_BROWSER_MAC_DICT_UTIL_H_

#import <Foundation/Foundation.h>

#include "base/values.h"

namespace electron {

NSArray* ListValueToNSArray(const base::ListValue& value);
base::ListValue NSArrayToValue(NSArray* arr);
NSDictionary* DictionaryValueToNSDictionary(const base::DictValue& value);
base::DictValue NSDictionaryToValue(NSDictionary* dict);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MAC_DICT_UTIL_H_
