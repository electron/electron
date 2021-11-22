// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MAC_DICT_UTIL_H_
#define ELECTRON_SHELL_BROWSER_MAC_DICT_UTIL_H_

#import <Foundation/Foundation.h>

namespace base {
class ListValue;
class DictionaryValue;
}  // namespace base

namespace electron {

NSArray* ListValueToNSArray(const base::ListValue& value);
base::ListValue NSArrayToListValue(NSArray* arr);
NSDictionary* DictionaryValueToNSDictionary(const base::DictionaryValue& value);
base::DictionaryValue NSDictionaryToDictionaryValue(NSDictionary* dict);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_MAC_DICT_UTIL_H_
