// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/mac/dict_util.h"

#include "base/mac/scoped_nsobject.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

namespace atom {

NSDictionary* DictionaryValueToNSDictionary(const base::DictionaryValue& value) {
  std::string json;
  if (!base::JSONWriter::Write(value, &json))
    return nil;
  NSData* jsonData = [NSData dataWithBytes:json.c_str() length:json.length()];
  id obj = [NSJSONSerialization JSONObjectWithData:jsonData
                                           options:0
                                             error:nil];
  if (![obj isKindOfClass:[NSDictionary class]])
    return nil;
  return obj;
}

scoped_ptr<base::DictionaryValue> NSDictionaryToDictionaryValue(
    NSDictionary* dict) {
  if (!dict)
    return nullptr;

  NSData* data = [NSJSONSerialization dataWithJSONObject:dict
                                                 options:0
                                                   error:nil];
  if (!data)
    return nullptr;

  base::scoped_nsobject<NSString> json =
      [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
  scoped_ptr<base::Value> value =
      base::JSONReader::Read([json UTF8String]);
  return base::DictionaryValue::From(std::move(value));
}

}  // namespace atom
