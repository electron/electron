// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#include <map>

#import <Cocoa/Cocoa.h>

#include "atom/browser/mac/dict_util.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "net/base/mac/url_conversions.h"

namespace atom {

namespace api {

namespace {

int g_next_id = 0;

// The map to convert |id| to |int|.
std::map<int, id> g_id_map;

}  // namespace

void SystemPreferences::PostNotification(const std::string& name,
    const base::DictionaryValue& user_info) {
  DoPostNotification(name, user_info, false);
}

void SystemPreferences::PostLocalNotification(const std::string& name,
    const base::DictionaryValue& user_info) {
  DoPostNotification(name, user_info, true);
}

void SystemPreferences::DoPostNotification(const std::string& name,
    const base::DictionaryValue& user_info, bool is_local) {
  NSNotificationCenter* center = is_local ?
    [NSNotificationCenter defaultCenter] :
    [NSDistributedNotificationCenter defaultCenter];
  [center
     postNotificationName:base::SysUTF8ToNSString(name)
                   object:nil
                 userInfo:DictionaryValueToNSDictionary(user_info)
  ];
}

int SystemPreferences::SubscribeNotification(
    const std::string& name, const NotificationCallback& callback) {
  return DoSubscribeNotification(name, callback, false);
}

void SystemPreferences::UnsubscribeNotification(int request_id) {
  DoUnsubscribeNotification(request_id, false);
}

int SystemPreferences::SubscribeLocalNotification(
    const std::string& name, const NotificationCallback& callback) {
  return DoSubscribeNotification(name, callback, true);
}

void SystemPreferences::UnsubscribeLocalNotification(int request_id) {
  DoUnsubscribeNotification(request_id, true);
}

int SystemPreferences::DoSubscribeNotification(const std::string& name,
  const NotificationCallback& callback, bool is_local) {
  int request_id = g_next_id++;
  __block NotificationCallback copied_callback = callback;
  NSNotificationCenter* center = is_local ?
    [NSNotificationCenter defaultCenter] :
    [NSDistributedNotificationCenter defaultCenter];

  g_id_map[request_id] = [center
    addObserverForName:base::SysUTF8ToNSString(name)
    object:nil
    queue:nil
    usingBlock:^(NSNotification* notification) {
      std::unique_ptr<base::DictionaryValue> user_info =
        NSDictionaryToDictionaryValue(notification.userInfo);
      if (user_info) {
        copied_callback.Run(
          base::SysNSStringToUTF8(notification.name),
          *user_info);
      } else {
        copied_callback.Run(
          base::SysNSStringToUTF8(notification.name),
          base::DictionaryValue());
      }
    }
  ];
  return request_id;
}

void SystemPreferences::DoUnsubscribeNotification(int request_id, bool is_local) {
  auto iter = g_id_map.find(request_id);
  if (iter != g_id_map.end()) {
    id observer = iter->second;
    NSNotificationCenter* center = is_local ?
      [NSNotificationCenter defaultCenter] :
      [NSDistributedNotificationCenter defaultCenter];
    [center removeObserver:observer];
    g_id_map.erase(iter);
  }
}

v8::Local<v8::Value> SystemPreferences::GetUserDefault(
    const std::string& name, const std::string& type) {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* key = base::SysUTF8ToNSString(name);
  if (type == "string") {
    return mate::StringToV8(isolate(),
      base::SysNSStringToUTF8([defaults stringForKey:key]));
  } else if (type == "boolean") {
    return v8::Boolean::New(isolate(), [defaults boolForKey:key]);
  } else if (type == "float") {
    return v8::Number::New(isolate(), [defaults floatForKey:key]);
  } else if (type == "integer") {
    return v8::Integer::New(isolate(), [defaults integerForKey:key]);
  } else if (type == "double") {
    return v8::Number::New(isolate(), [defaults doubleForKey:key]);
  } else if (type == "url") {
    return mate::ConvertToV8(isolate(),
      net::GURLWithNSURL([defaults URLForKey:key]));
  } else if (type == "array") {
    std::unique_ptr<base::ListValue> list =
        NSArrayToListValue([defaults arrayForKey:key]);
    if (list == nullptr)
      list.reset(new base::ListValue());
    return mate::ConvertToV8(isolate(), *list);
  } else if (type == "dictionary") {
    std::unique_ptr<base::DictionaryValue> dictionary =
        NSDictionaryToDictionaryValue([defaults dictionaryForKey:key]);
    if (dictionary == nullptr)
      dictionary.reset(new base::DictionaryValue());
    return mate::ConvertToV8(isolate(), *dictionary);
  } else {
    return v8::Undefined(isolate());
  }
}

void SystemPreferences::SetUserDefault(const std::string& name,
                                       const std::string& type,
                                       mate::Arguments* args) {
  const auto throwConversionError = [&] {
    args->ThrowError("Unable to convert value to: " + type);
  };

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* key = base::SysUTF8ToNSString(name);
  if (type == "string") {
    std::string value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    [defaults setObject:base::SysUTF8ToNSString(value) forKey:key];
  } else if (type == "boolean") {
    bool value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    [defaults setBool:value forKey:key];
  } else if (type == "float") {
    float value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    [defaults setFloat:value forKey:key];
  } else if (type == "integer") {
    int value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    [defaults setInteger:value forKey:key];
  } else if (type == "double") {
    double value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    [defaults setDouble:value forKey:key];
  } else if (type == "url") {
    GURL value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    if (NSURL* url = net::NSURLWithGURL(value)) {
      [defaults setURL:url forKey:key];
    }
  } else if (type == "array") {
    base::ListValue value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    if (NSArray* array = ListValueToNSArray(value)) {
      [defaults setObject:array forKey:key];
    }
  } else if (type == "dictionary") {
    base::DictionaryValue value;
    if (!args->GetNext(&value)) {
      throwConversionError();
      return;
    }

    if (NSDictionary* dict = DictionaryValueToNSDictionary(value)) {
      [defaults setObject:dict forKey:key];
    }
  } else {
    args->ThrowError("Invalid type: " + type);
    return;
  }
}

bool SystemPreferences::IsDarkMode() {
  NSString* mode = [[NSUserDefaults standardUserDefaults]
      stringForKey:@"AppleInterfaceStyle"];
  return [mode isEqualToString:@"Dark"];
}

bool SystemPreferences::IsSwipeTrackingFromScrollEventsEnabled() {
  return [NSEvent isSwipeTrackingFromScrollEventsEnabled];
}

}  // namespace api

}  // namespace atom
