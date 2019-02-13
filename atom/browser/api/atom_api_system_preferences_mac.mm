// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_system_preferences.h"

#include <map>

#import <AVFoundation/AVFoundation.h>
#import <Cocoa/Cocoa.h>
#import <LocalAuthentication/LocalAuthentication.h>
#import <Security/Security.h>

#include "atom/browser/mac/atom_application.h"
#include "atom/browser/mac/dict_util.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/values.h"
#include "native_mate/object_template_builder.h"
#include "net/base/mac/url_conversions.h"

namespace mate {
template <>
struct Converter<NSAppearance*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     NSAppearance** out) {
    if (val->IsNull()) {
      *out = nil;
      return true;
    }

    std::string name;
    if (!mate::ConvertFromV8(isolate, val, &name)) {
      return false;
    }

    if (name == "light") {
      *out = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
      return true;
    } else if (name == "dark") {
      if (@available(macOS 10.14, *)) {
        *out = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
      } else {
        *out = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
      }
      return true;
    }

    return false;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, NSAppearance* val) {
    if (val == nil) {
      return v8::Null(isolate);
    }

    if (val.name == NSAppearanceNameAqua) {
      return mate::ConvertToV8(isolate, "light");
    }
    if (@available(macOS 10.14, *)) {
      if (val.name == NSAppearanceNameDarkAqua) {
        return mate::ConvertToV8(isolate, "dark");
      }
    }

    return mate::ConvertToV8(isolate, "unknown");
  }
};
}  // namespace mate

namespace atom {

namespace api {

namespace {

int g_next_id = 0;

// The map to convert |id| to |int|.
std::map<int, id> g_id_map;

AVMediaType ParseMediaType(const std::string& media_type) {
  if (media_type == "camera") {
    return AVMediaTypeVideo;
  } else if (media_type == "microphone") {
    return AVMediaTypeAudio;
  } else {
    return nil;
  }
}

std::string ConvertAuthorizationStatus(AVAuthorizationStatusMac status) {
  switch (status) {
    case AVAuthorizationStatusNotDeterminedMac:
      return "not-determined";
    case AVAuthorizationStatusRestrictedMac:
      return "restricted";
    case AVAuthorizationStatusDeniedMac:
      return "denied";
    case AVAuthorizationStatusAuthorizedMac:
      return "granted";
    default:
      return "unknown";
  }
}

// Convert color to RGBA value like "aabbccdd"
std::string ToRGBA(NSColor* color) {
  return base::StringPrintf(
      "%02X%02X%02X%02X", (int)(color.redComponent * 0xFF),
      (int)(color.greenComponent * 0xFF), (int)(color.blueComponent * 0xFF),
      (int)(color.alphaComponent * 0xFF));
}

// Convert color to RGB hex value like "#ABCDEF"
std::string ToRGBHex(NSColor* color) {
  return base::StringPrintf("#%02X%02X%02X", (int)(color.redComponent * 0xFF),
                            (int)(color.greenComponent * 0xFF),
                            (int)(color.blueComponent * 0xFF));
}

}  // namespace

void SystemPreferences::PostNotification(const std::string& name,
                                         const base::DictionaryValue& user_info,
                                         mate::Arguments* args) {
  bool immediate = false;
  args->GetNext(&immediate);

  NSDistributedNotificationCenter* center =
      [NSDistributedNotificationCenter defaultCenter];
  [center postNotificationName:base::SysUTF8ToNSString(name)
                        object:nil
                      userInfo:DictionaryValueToNSDictionary(user_info)
            deliverImmediately:immediate];
}

int SystemPreferences::SubscribeNotification(
    const std::string& name,
    const NotificationCallback& callback) {
  return DoSubscribeNotification(name, callback,
                                 kNSDistributedNotificationCenter);
}

void SystemPreferences::UnsubscribeNotification(int request_id) {
  DoUnsubscribeNotification(request_id, kNSDistributedNotificationCenter);
}

void SystemPreferences::PostLocalNotification(
    const std::string& name,
    const base::DictionaryValue& user_info) {
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:base::SysUTF8ToNSString(name)
                        object:nil
                      userInfo:DictionaryValueToNSDictionary(user_info)];
}

int SystemPreferences::SubscribeLocalNotification(
    const std::string& name,
    const NotificationCallback& callback) {
  return DoSubscribeNotification(name, callback, kNSNotificationCenter);
}

void SystemPreferences::UnsubscribeLocalNotification(int request_id) {
  DoUnsubscribeNotification(request_id, kNSNotificationCenter);
}

void SystemPreferences::PostWorkspaceNotification(
    const std::string& name,
    const base::DictionaryValue& user_info) {
  NSNotificationCenter* center =
      [[NSWorkspace sharedWorkspace] notificationCenter];
  [center postNotificationName:base::SysUTF8ToNSString(name)
                        object:nil
                      userInfo:DictionaryValueToNSDictionary(user_info)];
}

int SystemPreferences::SubscribeWorkspaceNotification(
    const std::string& name,
    const NotificationCallback& callback) {
  return DoSubscribeNotification(name, callback,
                                 kNSWorkspaceNotificationCenter);
}

void SystemPreferences::UnsubscribeWorkspaceNotification(int request_id) {
  DoUnsubscribeNotification(request_id, kNSWorkspaceNotificationCenter);
}

int SystemPreferences::DoSubscribeNotification(
    const std::string& name,
    const NotificationCallback& callback,
    NotificationCenterKind kind) {
  int request_id = g_next_id++;
  __block NotificationCallback copied_callback = callback;
  NSNotificationCenter* center;
  switch (kind) {
    case kNSDistributedNotificationCenter:
      center = [NSDistributedNotificationCenter defaultCenter];
      break;
    case kNSNotificationCenter:
      center = [NSNotificationCenter defaultCenter];
      break;
    case kNSWorkspaceNotificationCenter:
      center = [[NSWorkspace sharedWorkspace] notificationCenter];
      break;
    default:
      break;
  }

  g_id_map[request_id] = [center
      addObserverForName:base::SysUTF8ToNSString(name)
                  object:nil
                   queue:nil
              usingBlock:^(NSNotification* notification) {
                std::unique_ptr<base::DictionaryValue> user_info =
                    NSDictionaryToDictionaryValue(notification.userInfo);
                if (user_info) {
                  copied_callback.Run(
                      base::SysNSStringToUTF8(notification.name), *user_info);
                } else {
                  copied_callback.Run(
                      base::SysNSStringToUTF8(notification.name),
                      base::DictionaryValue());
                }
              }];
  return request_id;
}

void SystemPreferences::DoUnsubscribeNotification(int request_id,
                                                  NotificationCenterKind kind) {
  auto iter = g_id_map.find(request_id);
  if (iter != g_id_map.end()) {
    id observer = iter->second;
    NSNotificationCenter* center;
    switch (kind) {
      case kNSDistributedNotificationCenter:
        center = [NSDistributedNotificationCenter defaultCenter];
        break;
      case kNSNotificationCenter:
        center = [NSNotificationCenter defaultCenter];
        break;
      case kNSWorkspaceNotificationCenter:
        center = [[NSWorkspace sharedWorkspace] notificationCenter];
        break;
      default:
        break;
    }
    [center removeObserver:observer];
    g_id_map.erase(iter);
  }
}

v8::Local<v8::Value> SystemPreferences::GetUserDefault(
    const std::string& name,
    const std::string& type) {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* key = base::SysUTF8ToNSString(name);
  if (type == "string") {
    return mate::StringToV8(
        isolate(), base::SysNSStringToUTF8([defaults stringForKey:key]));
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

void SystemPreferences::RegisterDefaults(mate::Arguments* args) {
  base::DictionaryValue value;

  if (!args->GetNext(&value)) {
    args->ThrowError("Invalid userDefault data provided");
  } else {
    @try {
      NSDictionary* dict = DictionaryValueToNSDictionary(value);
      for (id key in dict) {
        id value = [dict objectForKey:key];
        if ([value isKindOfClass:[NSNull class]] || value == nil) {
          args->ThrowError("Invalid userDefault data provided");
          return;
        }
      }
      [[NSUserDefaults standardUserDefaults] registerDefaults:dict];
    } @catch (NSException* exception) {
      args->ThrowError("Invalid userDefault data provided");
    }
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

std::string SystemPreferences::GetAccentColor() {
  NSColor* sysColor = nil;
  if (@available(macOS 10.14, *))
    sysColor = [NSColor controlAccentColor];

  return ToRGBA(sysColor);
}

std::string SystemPreferences::GetSystemColor(const std::string& color,
                                              mate::Arguments* args) {
  if (@available(macOS 10.10, *)) {
    NSColor* sysColor;
    if (color == "blue") {
      sysColor = [NSColor systemBlueColor];
    } else if (color == "brown") {
      sysColor = [NSColor systemBrownColor];
    } else if (color == "gray") {
      sysColor = [NSColor systemGrayColor];
    } else if (color == "green") {
      sysColor = [NSColor systemGreenColor];
    } else if (color == "orange") {
      sysColor = [NSColor systemOrangeColor];
    } else if (color == "pink") {
      sysColor = [NSColor systemPinkColor];
    } else if (color == "purple") {
      sysColor = [NSColor systemPurpleColor];
    } else if (color == "red") {
      sysColor = [NSColor systemRedColor];
    } else if (color == "yellow") {
      sysColor = [NSColor systemYellowColor];
    } else {
      args->ThrowError("Unknown system color: " + color);
      return "";
    }

    return ToRGBHex(sysColor);
  } else {
    args->ThrowError(
        "This api is not available on MacOS version 10.9 or lower.");
    return "";
  }
}

bool SystemPreferences::CanPromptTouchID() {
  if (@available(macOS 10.12.2, *)) {
    base::scoped_nsobject<LAContext> context([[LAContext alloc] init]);
    if (![context
            canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics
                        error:nil])
      return false;
    if (@available(macOS 10.13.2, *))
      return [context biometryType] == LABiometryTypeTouchID;
    return true;
  }
  return false;
}

void OnTouchIDCompleted(scoped_refptr<util::Promise> promise) {
  promise->Resolve();
}

void OnTouchIDFailed(scoped_refptr<util::Promise> promise,
                     const std::string& reason) {
  promise->RejectWithErrorMessage(reason);
}

v8::Local<v8::Promise> SystemPreferences::PromptTouchID(
    v8::Isolate* isolate,
    const std::string& reason) {
  scoped_refptr<util::Promise> promise = new util::Promise(isolate);
  if (@available(macOS 10.12.2, *)) {
    base::scoped_nsobject<LAContext> context([[LAContext alloc] init]);
    base::ScopedCFTypeRef<SecAccessControlRef> access_control =
        base::ScopedCFTypeRef<SecAccessControlRef>(
            SecAccessControlCreateWithFlags(
                kCFAllocatorDefault,
                kSecAttrAccessibleWhenUnlockedThisDeviceOnly,
                kSecAccessControlPrivateKeyUsage |
                    kSecAccessControlUserPresence,
                nullptr));

    scoped_refptr<base::SequencedTaskRunner> runner =
        base::SequencedTaskRunnerHandle::Get();

    [context
        evaluateAccessControl:access_control
                    operation:LAAccessControlOperationUseKeySign
              localizedReason:[NSString stringWithUTF8String:reason.c_str()]
                        reply:^(BOOL success, NSError* error) {
                          if (!success) {
                            runner->PostTask(
                                FROM_HERE,
                                base::BindOnce(
                                    &OnTouchIDFailed, promise,
                                    std::string([error.localizedDescription
                                                     UTF8String])));
                          } else {
                            runner->PostTask(
                                FROM_HERE,
                                base::BindOnce(&OnTouchIDCompleted, promise));
                          }
                        }];
  } else {
    promise->RejectWithErrorMessage(
        "This API is not available on macOS versions older than 10.12.2");
  }
  return promise->GetHandle();
}

// static
bool SystemPreferences::IsTrustedAccessibilityClient(bool prompt) {
  NSDictionary* options = @{(id)kAXTrustedCheckOptionPrompt : @(prompt)};
  return AXIsProcessTrustedWithOptions((CFDictionaryRef)options);
}

std::string SystemPreferences::GetColor(const std::string& color,
                                        mate::Arguments* args) {
  NSColor* sysColor = nil;
  if (color == "alternate-selected-control-text") {
    sysColor = [NSColor alternateSelectedControlTextColor];
  } else if (color == "control-background") {
    sysColor = [NSColor controlBackgroundColor];
  } else if (color == "control") {
    sysColor = [NSColor controlColor];
  } else if (color == "control-text") {
    sysColor = [NSColor controlTextColor];
  } else if (color == "disabled-control") {
    sysColor = [NSColor disabledControlTextColor];
  } else if (color == "find-highlight") {
    if (@available(macOS 10.14, *))
      sysColor = [NSColor findHighlightColor];
  } else if (color == "grid") {
    sysColor = [NSColor gridColor];
  } else if (color == "header-text") {
    sysColor = [NSColor headerTextColor];
  } else if (color == "highlight") {
    sysColor = [NSColor highlightColor];
  } else if (color == "keyboard-focus-indicator") {
    sysColor = [NSColor keyboardFocusIndicatorColor];
  } else if (color == "label") {
    if (@available(macOS 10.10, *))
      sysColor = [NSColor labelColor];
  } else if (color == "link") {
    if (@available(macOS 10.10, *))
      sysColor = [NSColor linkColor];
  } else if (color == "placeholder-text") {
    if (@available(macOS 10.10, *))
      sysColor = [NSColor placeholderTextColor];
  } else if (color == "quaternary-label") {
    if (@available(macOS 10.10, *))
      sysColor = [NSColor quaternaryLabelColor];
  } else if (color == "scrubber-textured-background") {
    if (@available(macOS 10.12.2, *))
      sysColor = [NSColor scrubberTexturedBackgroundColor];
  } else if (color == "secondary-label") {
    if (@available(macOS 10.10, *))
      sysColor = [NSColor secondaryLabelColor];
  } else if (color == "selected-content-background") {
    if (@available(macOS 10.14, *))
      sysColor = [NSColor selectedContentBackgroundColor];
  } else if (color == "selected-control") {
    sysColor = [NSColor selectedControlColor];
  } else if (color == "selected-control-text") {
    sysColor = [NSColor selectedControlTextColor];
  } else if (color == "selected-menu-item-text") {
    sysColor = [NSColor selectedMenuItemTextColor];
  } else if (color == "selected-text-background") {
    sysColor = [NSColor selectedTextBackgroundColor];
  } else if (color == "selected-text") {
    sysColor = [NSColor selectedTextColor];
  } else if (color == "separator") {
    if (@available(macOS 10.14, *))
      sysColor = [NSColor separatorColor];
  } else if (color == "shadow") {
    sysColor = [NSColor shadowColor];
  } else if (color == "tertiary-label") {
    if (@available(macOS 10.10, *))
      sysColor = [NSColor tertiaryLabelColor];
  } else if (color == "text-background") {
    sysColor = [NSColor textBackgroundColor];
  } else if (color == "text") {
    sysColor = [NSColor textColor];
  } else if (color == "under-page-background") {
    sysColor = [NSColor underPageBackgroundColor];
  } else if (color == "unemphasized-selected-content-background") {
    if (@available(macOS 10.14, *))
      sysColor = [NSColor unemphasizedSelectedContentBackgroundColor];
  } else if (color == "unemphasized-selected-text-background") {
    if (@available(macOS 10.14, *))
      sysColor = [NSColor unemphasizedSelectedTextBackgroundColor];
  } else if (color == "unemphasized-selected-text") {
    if (@available(macOS 10.14, *))
      sysColor = [NSColor unemphasizedSelectedTextColor];
  } else if (color == "window-background") {
    sysColor = [NSColor windowBackgroundColor];
  } else if (color == "window-frame-text") {
    sysColor = [NSColor windowFrameTextColor];
  } else {
    args->ThrowError("Unknown color: " + color);
    return "";
  }

  return ToRGBHex(sysColor);
}

std::string SystemPreferences::GetMediaAccessStatus(
    const std::string& media_type,
    mate::Arguments* args) {
  if (auto type = ParseMediaType(media_type)) {
    if (@available(macOS 10.14, *)) {
      return ConvertAuthorizationStatus(
          [AVCaptureDevice authorizationStatusForMediaType:type]);
    } else {
      // access always allowed pre-10.14 Mojave
      return ConvertAuthorizationStatus(AVAuthorizationStatusAuthorizedMac);
    }
  } else {
    args->ThrowError("Invalid media type");
    return std::string();
  }
}

v8::Local<v8::Promise> SystemPreferences::AskForMediaAccess(
    v8::Isolate* isolate,
    const std::string& media_type) {
  scoped_refptr<util::Promise> promise = new util::Promise(isolate);

  if (auto type = ParseMediaType(media_type)) {
    if (@available(macOS 10.14, *)) {
      [AVCaptureDevice requestAccessForMediaType:type
                               completionHandler:^(BOOL granted) {
                                 dispatch_async(dispatch_get_main_queue(), ^{
                                   promise->Resolve(!!granted);
                                 });
                               }];
    } else {
      // access always allowed pre-10.14 Mojave
      promise->Resolve(true);
    }
  } else {
    promise->RejectWithErrorMessage("Invalid media type");
  }

  return promise->GetHandle();
}

void SystemPreferences::RemoveUserDefault(const std::string& name) {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults removeObjectForKey:base::SysUTF8ToNSString(name)];
}

bool SystemPreferences::IsDarkMode() {
  NSString* mode = [[NSUserDefaults standardUserDefaults]
      stringForKey:@"AppleInterfaceStyle"];
  return [mode isEqualToString:@"Dark"];
}

bool SystemPreferences::IsSwipeTrackingFromScrollEventsEnabled() {
  return [NSEvent isSwipeTrackingFromScrollEventsEnabled];
}

v8::Local<v8::Value> SystemPreferences::GetEffectiveAppearance(
    v8::Isolate* isolate) {
  if (@available(macOS 10.14, *)) {
    return mate::ConvertToV8(
        isolate, [NSApplication sharedApplication].effectiveAppearance);
  }
  return v8::Null(isolate);
}

v8::Local<v8::Value> SystemPreferences::GetAppLevelAppearance(
    v8::Isolate* isolate) {
  if (@available(macOS 10.14, *)) {
    return mate::ConvertToV8(isolate,
                             [NSApplication sharedApplication].appearance);
  }
  return v8::Null(isolate);
}

void SystemPreferences::SetAppLevelAppearance(mate::Arguments* args) {
  if (@available(macOS 10.14, *)) {
    NSAppearance* appearance;
    if (args->GetNext(&appearance)) {
      [[NSApplication sharedApplication] setAppearance:appearance];
    } else {
      args->ThrowError("Invalid app appearance provided as first argument");
    }
  }
}

}  // namespace api

}  // namespace atom
