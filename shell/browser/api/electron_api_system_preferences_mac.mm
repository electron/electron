// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_system_preferences.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#import <AVFoundation/AVFoundation.h>
#import <Cocoa/Cocoa.h>
#import <LocalAuthentication/LocalAuthentication.h>
#import <Security/Security.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/media/webrtc/system_media_capture_permissions_mac.h"
#include "net/base/mac/url_conversions.h"
#include "shell/browser/mac/dict_util.h"
#include "shell/browser/mac/electron_application.h"
#include "shell/browser/ui/cocoa/NSColor+Hex.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/native_theme/native_theme.h"

namespace gin {

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
    if (!gin::ConvertFromV8(isolate, val, &name)) {
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

    if ([val.name isEqualToString:NSAppearanceNameAqua]) {
      return gin::ConvertToV8(isolate, "light");
    }
    if (@available(macOS 10.14, *)) {
      if ([val.name isEqualToString:NSAppearanceNameDarkAqua]) {
        return gin::ConvertToV8(isolate, "dark");
      }
    }

    return gin::ConvertToV8(isolate, "unknown");
  }
};

}  // namespace gin

namespace electron {

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

std::string ConvertSystemPermission(
    system_media_permissions::SystemPermission value) {
  using SystemPermission = system_media_permissions::SystemPermission;
  switch (value) {
    case SystemPermission::kNotDetermined:
      return "not-determined";
    case SystemPermission::kRestricted:
      return "restricted";
    case SystemPermission::kDenied:
      return "denied";
    case SystemPermission::kAllowed:
      return "granted";
    default:
      return "unknown";
  }
}

NSNotificationCenter* GetNotificationCenter(NotificationCenterKind kind) {
  switch (kind) {
    case NotificationCenterKind::kNSDistributedNotificationCenter:
      return [NSDistributedNotificationCenter defaultCenter];
    case NotificationCenterKind::kNSNotificationCenter:
      return [NSNotificationCenter defaultCenter];
    case NotificationCenterKind::kNSWorkspaceNotificationCenter:
      return [[NSWorkspace sharedWorkspace] notificationCenter];
    default:
      return nil;
  }
}

}  // namespace

void SystemPreferences::PostNotification(const std::string& name,
                                         base::DictionaryValue user_info,
                                         gin::Arguments* args) {
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
  return DoSubscribeNotification(
      name, callback, NotificationCenterKind::kNSDistributedNotificationCenter);
}

void SystemPreferences::UnsubscribeNotification(int request_id) {
  DoUnsubscribeNotification(
      request_id, NotificationCenterKind::kNSDistributedNotificationCenter);
}

void SystemPreferences::PostLocalNotification(const std::string& name,
                                              base::DictionaryValue user_info) {
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:base::SysUTF8ToNSString(name)
                        object:nil
                      userInfo:DictionaryValueToNSDictionary(user_info)];
}

int SystemPreferences::SubscribeLocalNotification(
    const std::string& name,
    const NotificationCallback& callback) {
  return DoSubscribeNotification(name, callback,
                                 NotificationCenterKind::kNSNotificationCenter);
}

void SystemPreferences::UnsubscribeLocalNotification(int request_id) {
  DoUnsubscribeNotification(request_id,
                            NotificationCenterKind::kNSNotificationCenter);
}

void SystemPreferences::PostWorkspaceNotification(
    const std::string& name,
    base::DictionaryValue user_info) {
  NSNotificationCenter* center =
      [[NSWorkspace sharedWorkspace] notificationCenter];
  [center postNotificationName:base::SysUTF8ToNSString(name)
                        object:nil
                      userInfo:DictionaryValueToNSDictionary(user_info)];
}

int SystemPreferences::SubscribeWorkspaceNotification(
    const std::string& name,
    const NotificationCallback& callback) {
  return DoSubscribeNotification(
      name, callback, NotificationCenterKind::kNSWorkspaceNotificationCenter);
}

void SystemPreferences::UnsubscribeWorkspaceNotification(int request_id) {
  DoUnsubscribeNotification(
      request_id, NotificationCenterKind::kNSWorkspaceNotificationCenter);
}

int SystemPreferences::DoSubscribeNotification(
    const std::string& name,
    const NotificationCallback& callback,
    NotificationCenterKind kind) {
  int request_id = g_next_id++;
  __block NotificationCallback copied_callback = callback;

  g_id_map[request_id] = [GetNotificationCenter(kind)
      addObserverForName:base::SysUTF8ToNSString(name)
                  object:nil
                   queue:nil
              usingBlock:^(NSNotification* notification) {
                std::string object = "";
                if ([notification.object isKindOfClass:[NSString class]]) {
                  object = base::SysNSStringToUTF8(notification.object);
                }

                if (notification.userInfo) {
                  copied_callback.Run(
                      base::SysNSStringToUTF8(notification.name),
                      NSDictionaryToDictionaryValue(notification.userInfo),
                      object);
                } else {
                  copied_callback.Run(
                      base::SysNSStringToUTF8(notification.name),
                      base::DictionaryValue(), object);
                }
              }];
  return request_id;
}

void SystemPreferences::DoUnsubscribeNotification(int request_id,
                                                  NotificationCenterKind kind) {
  auto iter = g_id_map.find(request_id);
  if (iter != g_id_map.end()) {
    id observer = iter->second;
    [GetNotificationCenter(kind) removeObserver:observer];
    g_id_map.erase(iter);
  }
}

v8::Local<v8::Value> SystemPreferences::GetUserDefault(
    v8::Isolate* isolate,
    const std::string& name,
    const std::string& type) {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* key = base::SysUTF8ToNSString(name);
  if (type == "string") {
    return gin::StringToV8(
        isolate, base::SysNSStringToUTF8([defaults stringForKey:key]));
  } else if (type == "boolean") {
    return v8::Boolean::New(isolate, [defaults boolForKey:key]);
  } else if (type == "float") {
    return v8::Number::New(isolate, [defaults floatForKey:key]);
  } else if (type == "integer") {
    return v8::Integer::New(isolate, [defaults integerForKey:key]);
  } else if (type == "double") {
    return v8::Number::New(isolate, [defaults doubleForKey:key]);
  } else if (type == "url") {
    return gin::ConvertToV8(isolate,
                            net::GURLWithNSURL([defaults URLForKey:key]));
  } else if (type == "array") {
    return gin::ConvertToV8(isolate,
                            NSArrayToListValue([defaults arrayForKey:key]));
  } else if (type == "dictionary") {
    return gin::ConvertToV8(isolate, NSDictionaryToDictionaryValue(
                                         [defaults dictionaryForKey:key]));
  } else {
    return v8::Undefined(isolate);
  }
}

void SystemPreferences::RegisterDefaults(gin::Arguments* args) {
  base::DictionaryValue value;

  if (!args->GetNext(&value)) {
    args->ThrowError();
  } else {
    @try {
      NSDictionary* dict = DictionaryValueToNSDictionary(value);
      for (id key in dict) {
        id value = [dict objectForKey:key];
        if ([value isKindOfClass:[NSNull class]] || value == nil) {
          args->ThrowError();
          return;
        }
      }
      [[NSUserDefaults standardUserDefaults] registerDefaults:dict];
    } @catch (NSException* exception) {
      args->ThrowError();
    }
  }
}

void SystemPreferences::SetUserDefault(const std::string& name,
                                       const std::string& type,
                                       gin::Arguments* args) {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* key = base::SysUTF8ToNSString(name);
  if (type == "string") {
    std::string value;
    if (args->GetNext(&value)) {
      [defaults setObject:base::SysUTF8ToNSString(value) forKey:key];
      return;
    }
  } else if (type == "boolean") {
    bool value;
    v8::Local<v8::Value> next = args->PeekNext();
    if (!next.IsEmpty() && next->IsBoolean() && args->GetNext(&value)) {
      [defaults setBool:value forKey:key];
      return;
    }
  } else if (type == "float") {
    float value;
    if (args->GetNext(&value)) {
      [defaults setFloat:value forKey:key];
      return;
    }
  } else if (type == "integer") {
    int value;
    if (args->GetNext(&value)) {
      [defaults setInteger:value forKey:key];
      return;
    }
  } else if (type == "double") {
    double value;
    if (args->GetNext(&value)) {
      [defaults setDouble:value forKey:key];
      return;
    }
  } else if (type == "url") {
    GURL value;
    if (args->GetNext(&value)) {
      if (NSURL* url = net::NSURLWithGURL(value)) {
        [defaults setURL:url forKey:key];
        return;
      }
    }
  } else if (type == "array") {
    base::ListValue value;
    if (args->GetNext(&value)) {
      if (NSArray* array = ListValueToNSArray(value)) {
        [defaults setObject:array forKey:key];
        return;
      }
    }
  } else if (type == "dictionary") {
    base::DictionaryValue value;
    if (args->GetNext(&value)) {
      if (NSDictionary* dict = DictionaryValueToNSDictionary(value)) {
        [defaults setObject:dict forKey:key];
        return;
      }
    }
  } else {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowTypeError("Invalid type: " + type);
    return;
  }

  gin_helper::ErrorThrower(args->isolate())
      .ThrowTypeError("Unable to convert value to: " + type);
}

std::string SystemPreferences::GetAccentColor() {
  NSColor* sysColor = nil;
  if (@available(macOS 10.14, *))
    sysColor = [NSColor controlAccentColor];

  return ToRGBAHex(skia::NSSystemColorToSkColor(sysColor),
                   false /* include_hash */);
}

std::string SystemPreferences::GetSystemColor(gin_helper::ErrorThrower thrower,
                                              const std::string& color) {
  NSColor* sysColor = nil;
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
    thrower.ThrowError("Unknown system color: " + color);
    return "";
  }

  return ToRGBAHex(skia::NSSystemColorToSkColor(sysColor));
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

v8::Local<v8::Promise> SystemPreferences::PromptTouchID(
    v8::Isolate* isolate,
    const std::string& reason) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

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

    __block gin_helper::Promise<void> p = std::move(promise);
    [context
        evaluateAccessControl:access_control
                    operation:LAAccessControlOperationUseKeySign
              localizedReason:[NSString stringWithUTF8String:reason.c_str()]
                        reply:^(BOOL success, NSError* error) {
                          if (!success) {
                            std::string err_msg = std::string(
                                [error.localizedDescription UTF8String]);
                            runner->PostTask(
                                FROM_HERE,
                                base::BindOnce(
                                    gin_helper::Promise<void>::RejectPromise,
                                    std::move(p), std::move(err_msg)));
                          } else {
                            runner->PostTask(
                                FROM_HERE,
                                base::BindOnce(
                                    gin_helper::Promise<void>::ResolvePromise,
                                    std::move(p)));
                          }
                        }];
  } else {
    promise.RejectWithErrorMessage(
        "This API is not available on macOS versions older than 10.12.2");
  }
  return handle;
}

// static
bool SystemPreferences::IsTrustedAccessibilityClient(bool prompt) {
  NSDictionary* options = @{(id)kAXTrustedCheckOptionPrompt : @(prompt)};
  return AXIsProcessTrustedWithOptions((CFDictionaryRef)options);
}

std::string SystemPreferences::GetColor(gin_helper::ErrorThrower thrower,
                                        const std::string& color) {
  NSColor* sysColor = nil;
  if (color == "alternate-selected-control-text") {
    sysColor = [NSColor alternateSelectedControlTextColor];
    EmitWarning(
        node::Environment::GetCurrent(thrower.isolate()),
        "'alternate-selected-control-text' is deprecated as an input to "
        "getColor.  Use 'selected-content-background' instead.",
        "electron");
  } else if (color == "control-background") {
    sysColor = [NSColor controlBackgroundColor];
  } else if (color == "control") {
    sysColor = [NSColor controlColor];
  } else if (color == "control-text") {
    sysColor = [NSColor controlTextColor];
  } else if (color == "disabled-control-text") {
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
    sysColor = [NSColor labelColor];
  } else if (color == "link") {
    sysColor = [NSColor linkColor];
  } else if (color == "placeholder-text") {
    sysColor = [NSColor placeholderTextColor];
  } else if (color == "quaternary-label") {
    sysColor = [NSColor quaternaryLabelColor];
  } else if (color == "scrubber-textured-background") {
    if (@available(macOS 10.12.2, *))
      sysColor = [NSColor scrubberTexturedBackgroundColor];
  } else if (color == "secondary-label") {
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
    thrower.ThrowError("Unknown color: " + color);
  }

  if (sysColor)
    return ToRGBHex(skia::NSSystemColorToSkColor(sysColor));
  return "";
}

std::string SystemPreferences::GetMediaAccessStatus(
    gin_helper::ErrorThrower thrower,
    const std::string& media_type) {
  if (media_type == "camera") {
    return ConvertSystemPermission(
        system_media_permissions::CheckSystemVideoCapturePermission());
  } else if (media_type == "microphone") {
    return ConvertSystemPermission(
        system_media_permissions::CheckSystemAudioCapturePermission());
  } else if (media_type == "screen") {
    return ConvertSystemPermission(
        system_media_permissions::CheckSystemScreenCapturePermission());
  } else {
    thrower.ThrowError("Invalid media type");
    return std::string();
  }
}

v8::Local<v8::Promise> SystemPreferences::AskForMediaAccess(
    v8::Isolate* isolate,
    const std::string& media_type) {
  gin_helper::Promise<bool> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (auto type = ParseMediaType(media_type)) {
    if (@available(macOS 10.14, *)) {
      __block gin_helper::Promise<bool> p = std::move(promise);
      [AVCaptureDevice requestAccessForMediaType:type
                               completionHandler:^(BOOL granted) {
                                 dispatch_async(dispatch_get_main_queue(), ^{
                                   p.Resolve(!!granted);
                                 });
                               }];
    } else {
      // access always allowed pre-10.14 Mojave
      promise.Resolve(true);
    }
  } else {
    promise.RejectWithErrorMessage("Invalid media type");
  }

  return handle;
}

void SystemPreferences::RemoveUserDefault(const std::string& name) {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults removeObjectForKey:base::SysUTF8ToNSString(name)];
}

bool SystemPreferences::IsSwipeTrackingFromScrollEventsEnabled() {
  return [NSEvent isSwipeTrackingFromScrollEventsEnabled];
}

v8::Local<v8::Value> SystemPreferences::GetEffectiveAppearance(
    v8::Isolate* isolate) {
  if (@available(macOS 10.14, *)) {
    return gin::ConvertToV8(
        isolate, [NSApplication sharedApplication].effectiveAppearance);
  }
  return v8::Null(isolate);
}

v8::Local<v8::Value> SystemPreferences::GetAppLevelAppearance(
    v8::Isolate* isolate) {
  if (@available(macOS 10.14, *)) {
    return gin::ConvertToV8(isolate,
                            [NSApplication sharedApplication].appearance);
  }
  return v8::Null(isolate);
}

void SystemPreferences::SetAppLevelAppearance(gin::Arguments* args) {
  if (@available(macOS 10.14, *)) {
    NSAppearance* appearance;
    if (args->GetNext(&appearance)) {
      [[NSApplication sharedApplication] setAppearance:appearance];
    } else {
      args->ThrowError();
    }
  }
}

}  // namespace api

}  // namespace electron
