// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_MAC_DICT_UTIL_H_
#define ATOM_BROWSER_MAC_DICT_UTIL_H_

#include <memory>

#import <Foundation/Foundation.h>

namespace base {
class ListValue;
class DictionaryValue;
}

namespace atom {

NSArray* ListValueToNSArray(const base::ListValue& value);

std::unique_ptr<base::ListValue> NSArrayToListValue(NSArray* arr);

NSDictionary* DictionaryValueToNSDictionary(const base::DictionaryValue& value);

std::unique_ptr<base::DictionaryValue> NSDictionaryToDictionaryValue(
    NSDictionary* dict);

}  // namespace atom

#endif  // ATOM_BROWSER_MAC_DICT_UTIL_H_
