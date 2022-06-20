// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/mac/dict_util.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"

namespace electron {

NSArray* ListValueToNSArray(const base::Value::List& value) {
  std::string json;
  if (!base::JSONWriter::Write(base::Value(value.Clone()), &json))
    return nil;
  NSData* jsonData = [NSData dataWithBytes:json.c_str() length:json.length()];
  id obj = [NSJSONSerialization JSONObjectWithData:jsonData
                                           options:0
                                             error:nil];
  if (![obj isKindOfClass:[NSArray class]])
    return nil;
  return obj;
}

base::Value::List NSArrayToValue(NSArray* arr) {
  base::Value::List result;
  if (!arr)
    return result;

  for (id value in arr) {
    if ([value isKindOfClass:[NSString class]]) {
      result.Append(base::SysNSStringToUTF8(value));
    } else if ([value isKindOfClass:[NSNumber class]]) {
      const char* objc_type = [value objCType];
      if (strcmp(objc_type, @encode(BOOL)) == 0 ||
          strcmp(objc_type, @encode(char)) == 0)
        result.Append([value boolValue]);
      else if (strcmp(objc_type, @encode(double)) == 0 ||
               strcmp(objc_type, @encode(float)) == 0)
        result.Append([value doubleValue]);
      else
        result.Append([value intValue]);
    } else if ([value isKindOfClass:[NSArray class]]) {
      result.Append(NSArrayToValue(value));
    } else if ([value isKindOfClass:[NSDictionary class]]) {
      result.Append(NSDictionaryToValue(value));
    } else {
      result.Append(base::SysNSStringToUTF8([value description]));
    }
  }

  return result;
}

NSDictionary* DictionaryValueToNSDictionary(const base::Value::Dict& value) {
  std::string json;
  if (!base::JSONWriter::Write(base::Value(value.Clone()), &json))
    return nil;
  NSData* jsonData = [NSData dataWithBytes:json.c_str() length:json.length()];
  id obj = [NSJSONSerialization JSONObjectWithData:jsonData
                                           options:0
                                             error:nil];
  if (![obj isKindOfClass:[NSDictionary class]])
    return nil;
  return obj;
}

base::Value::Dict NSDictionaryToValue(NSDictionary* dict) {
  base::Value::Dict result;
  if (!dict)
    return result;

  for (id key in dict) {
    std::string str_key = base::SysNSStringToUTF8(
        [key isKindOfClass:[NSString class]] ? key : [key description]);

    id value = [dict objectForKey:key];
    if ([value isKindOfClass:[NSString class]]) {
      result.Set(str_key, base::Value(base::SysNSStringToUTF8(value)));
    } else if ([value isKindOfClass:[NSNumber class]]) {
      const char* objc_type = [value objCType];
      if (strcmp(objc_type, @encode(BOOL)) == 0 ||
          strcmp(objc_type, @encode(char)) == 0)
        result.Set(str_key, base::Value([value boolValue]));
      else if (strcmp(objc_type, @encode(double)) == 0 ||
               strcmp(objc_type, @encode(float)) == 0)
        result.Set(str_key, base::Value([value doubleValue]));
      else
        result.Set(str_key, base::Value([value intValue]));
    } else if ([value isKindOfClass:[NSArray class]]) {
      result.Set(str_key, NSArrayToValue(value));
    } else if ([value isKindOfClass:[NSDictionary class]]) {
      result.Set(str_key, NSDictionaryToValue(value));
    } else {
      result.Set(str_key,
                 base::Value(base::SysNSStringToUTF8([value description])));
    }
  }

  return result;
}

}  // namespace electron
