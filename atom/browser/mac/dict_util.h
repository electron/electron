// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#import <Foundation/Foundation.h>

namespace base {
class ListValue;
class DictionaryValue;
}

namespace atom {

NSDictionary* DictionaryValueToNSDictionary(const base::DictionaryValue& value);

std::unique_ptr<base::DictionaryValue> NSDictionaryToDictionaryValue(
    NSDictionary* dict);

std::unique_ptr<base::ListValue> NSArrayToListValue(NSArray* arr);

}  // namespace atom
