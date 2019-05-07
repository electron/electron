// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/browser.h"

#include <memory>
#include <string>
#include <utility>

#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "net/base/mac/url_conversions.h"
#include "shell/browser/badging/badge_manager.h"
#include "shell/browser/mac/dict_util.h"
#include "shell/browser/mac/electron_application.h"
#include "shell/browser/mac/electron_application_delegate.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/platform_util.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace electron {

namespace {

NSString* GetAppPathForProtocol(const GURL& url) {
  NSURL* ns_url = [NSURL
      URLWithString:base::SysUTF8ToNSString(url.possibly_invalid_spec())];
  base::ScopedCFTypeRef<CFErrorRef> out_err;

  base::ScopedCFTypeRef<CFURLRef> openingApp(LSCopyDefaultApplicationURLForURL(
      (CFURLRef)ns_url, kLSRolesAll, out_err.InitializeInto()));

  if (out_err) {
    // likely kLSApplicationNotFoundErr
    return nullptr;
  }
  NSString* app_path = [base::mac::CFToNSCast(openingApp.get()) path];
  return app_path;
}

gfx::Image GetApplicationIconForProtocol(NSString* _Nonnull app_path) {
  NSImage* image = [[NSWorkspace sharedWorkspace] iconForFile:app_path];
  gfx::Image icon(image);
  return icon;
}

std::u16string GetAppDisplayNameForProtocol(NSString* app_path) {
  NSString* app_display_name =
      [[NSFileManager defaultManager] displayNameAtPath:app_path];
  return base::SysNSStringToUTF16(app_display_name);
}

}  // namespace

v8::Local<v8::Promise> Browser::GetApplicationInfoForProtocol(
    v8::Isolate* isolate,
    const GURL& url) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);

  NSString* ns_app_path = GetAppPathForProtocol(url);

  if (!ns_app_path) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return handle;
  }

  std::u16string app_path = base::SysNSStringToUTF16(ns_app_path);
  std::u16string app_display_name = GetAppDisplayNameForProtocol(ns_app_path);
  gfx::Image app_icon = GetApplicationIconForProtocol(ns_app_path);

  dict.Set("name", app_display_name);
  dict.Set("path", app_path);
  dict.Set("icon", app_icon);

  promise.Resolve(dict);
  return handle;
}

void Browser::SetShutdownHandler(base::RepeatingCallback<bool()> handler) {
  [[AtomApplication sharedApplication] setShutdownHandler:std::move(handler)];
}

void Browser::Focus(gin::Arguments* args) {
  gin_helper::Dictionary opts;
  bool steal_focus = false;

  if (args->GetNext(&opts)) {
    gin_helper::ErrorThrower thrower(args->isolate());
    if (!opts.Get("steal", &steal_focus)) {
      thrower.ThrowError(
          "Expected options object to contain a 'steal' boolean property");
      return;
    }
  }

  [[AtomApplication sharedApplication] activateIgnoringOtherApps:steal_focus];
}

void Browser::Hide() {
  [[AtomApplication sharedApplication] hide:nil];
}

void Browser::Show() {
  [[AtomApplication sharedApplication] unhide:nil];
}

void Browser::AddRecentDocument(const base::FilePath& path) {
  NSString* path_string = base::mac::FilePathToNSString(path);
  if (!path_string)
    return;
  NSURL* u = [NSURL fileURLWithPath:path_string];
  if (!u)
    return;
  [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:u];
}

void Browser::ClearRecentDocuments() {
  [[NSDocumentController sharedDocumentController] clearRecentDocuments:nil];
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            gin::Arguments* args) {
  NSString* identifier = [base::mac::MainBundle() bundleIdentifier];
  if (!identifier)
    return false;

  if (!Browser::IsDefaultProtocolClient(protocol, args))
    return false;

  NSString* protocol_ns = [NSString stringWithUTF8String:protocol.c_str()];
  CFStringRef protocol_cf = base::mac::NSToCFCast(protocol_ns);
  CFArrayRef bundleList = LSCopyAllHandlersForURLScheme(protocol_cf);
  if (!bundleList) {
    return false;
  }
  // On macOS, we can't query the default, but the handlers list seems to put
  // Apple's defaults first, so we'll use the first option that isn't our bundle
  CFStringRef other = nil;
  for (CFIndex i = 0; i < CFArrayGetCount(bundleList); ++i) {
    other =
        base::mac::CFCast<CFStringRef>(CFArrayGetValueAtIndex(bundleList, i));
    if (![identifier isEqualToString:(__bridge NSString*)other]) {
      break;
    }
  }

  // No other app was found set it to none instead of setting it back to itself.
  if ([identifier isEqualToString:(__bridge NSString*)other]) {
    other = base::mac::NSToCFCast(@"None");
  }

  OSStatus return_code = LSSetDefaultHandlerForURLScheme(protocol_cf, other);
  return return_code == noErr;
}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                         gin::Arguments* args) {
  if (protocol.empty())
    return false;

  NSString* identifier = [base::mac::MainBundle() bundleIdentifier];
  if (!identifier)
    return false;

  NSString* protocol_ns = [NSString stringWithUTF8String:protocol.c_str()];
  OSStatus return_code = LSSetDefaultHandlerForURLScheme(
      base::mac::NSToCFCast(protocol_ns), base::mac::NSToCFCast(identifier));
  return return_code == noErr;
}

bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      gin::Arguments* args) {
  if (protocol.empty())
    return false;

  NSString* identifier = [base::mac::MainBundle() bundleIdentifier];
  if (!identifier)
    return false;

  NSString* protocol_ns = [NSString stringWithUTF8String:protocol.c_str()];

  base::ScopedCFTypeRef<CFStringRef> bundleId(
      LSCopyDefaultHandlerForURLScheme(base::mac::NSToCFCast(protocol_ns)));

  if (!bundleId)
    return false;

  // Ensure the comparison is case-insensitive
  // as LS does not persist the case of the bundle id.
  NSComparisonResult result =
      [base::mac::CFToNSCast(bundleId) caseInsensitiveCompare:identifier];
  return result == NSOrderedSame;
}

std::u16string Browser::GetApplicationNameForProtocol(const GURL& url) {
  NSString* app_path = GetAppPathForProtocol(url);
  if (!app_path) {
    return std::u16string();
  }
  std::u16string app_display_name = GetAppDisplayNameForProtocol(app_path);
  return app_display_name;
}

bool Browser::SetBadgeCount(absl::optional<int> count) {
  DockSetBadgeText(!count.has_value() || count.value() != 0
                       ? badging::BadgeManager::GetBadgeString(count)
                       : "");
  if (count.has_value()) {
    badge_count_ = count.value();
  } else {
    badge_count_ = 0;
  }
  return true;
}

void Browser::SetUserActivity(const std::string& type,
                              base::DictionaryValue user_info,
                              gin::Arguments* args) {
  std::string url_string;
  args->GetNext(&url_string);

  [[AtomApplication sharedApplication]
      setCurrentActivity:base::SysUTF8ToNSString(type)
            withUserInfo:DictionaryValueToNSDictionary(user_info)
          withWebpageURL:net::NSURLWithGURL(GURL(url_string))];
}

std::string Browser::GetCurrentActivityType() {
  NSUserActivity* userActivity =
      [[AtomApplication sharedApplication] getCurrentActivity];
  return base::SysNSStringToUTF8(userActivity.activityType);
}

void Browser::InvalidateCurrentActivity() {
  [[AtomApplication sharedApplication] invalidateCurrentActivity];
}

void Browser::ResignCurrentActivity() {
  [[AtomApplication sharedApplication] resignCurrentActivity];
}

void Browser::UpdateCurrentActivity(const std::string& type,
                                    base::DictionaryValue user_info) {
  [[AtomApplication sharedApplication]
      updateCurrentActivity:base::SysUTF8ToNSString(type)
               withUserInfo:DictionaryValueToNSDictionary(user_info)];
}

bool Browser::WillContinueUserActivity(const std::string& type) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnWillContinueUserActivity(&prevent_default, type);
  return prevent_default;
}

void Browser::DidFailToContinueUserActivity(const std::string& type,
                                            const std::string& error) {
  for (BrowserObserver& observer : observers_)
    observer.OnDidFailToContinueUserActivity(type, error);
}

bool Browser::ContinueUserActivity(const std::string& type,
                                   base::DictionaryValue user_info,
                                   base::DictionaryValue details) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnContinueUserActivity(&prevent_default, type, user_info, details);
  return prevent_default;
}

void Browser::UserActivityWasContinued(const std::string& type,
                                       base::DictionaryValue user_info) {
  for (BrowserObserver& observer : observers_)
    observer.OnUserActivityWasContinued(type, user_info);
}

bool Browser::UpdateUserActivityState(const std::string& type,
                                      base::DictionaryValue user_info) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnUpdateUserActivityState(&prevent_default, type, user_info);
  return prevent_default;
}

Browser::LoginItemSettings Browser::GetLoginItemSettings(
    const LoginItemSettings& options) {
  LoginItemSettings settings;
#if defined(MAS_BUILD)
  settings.open_at_login = platform_util::GetLoginItemEnabled();
#else
  settings.open_at_login =
      base::mac::CheckLoginItemStatus(&settings.open_as_hidden);
  settings.restore_state = base::mac::WasLaunchedAsLoginItemRestoreState();
  settings.opened_at_login = base::mac::WasLaunchedAsLoginOrResumeItem();
  settings.opened_as_hidden = base::mac::WasLaunchedAsHiddenLoginItem();
#endif
  return settings;
}

void RemoveFromLoginItems() {
#pragma clang diagnostic push  // https://crbug.com/1154377
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  // logic to find the login item copied from GetLoginItemForApp in
  // base/mac/mac_util.mm
  base::ScopedCFTypeRef<LSSharedFileListRef> login_items(
      LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL));
  if (!login_items.get()) {
    LOG(ERROR) << "Couldn't get a Login Items list.";
    return;
  }
  base::scoped_nsobject<NSArray> login_items_array(
      base::mac::CFToNSCast(LSSharedFileListCopySnapshot(login_items, NULL)));
  NSURL* url = [NSURL fileURLWithPath:[base::mac::MainBundle() bundlePath]];
  for (NSUInteger i = 0; i < [login_items_array count]; ++i) {
    LSSharedFileListItemRef item =
        reinterpret_cast<LSSharedFileListItemRef>(login_items_array[i]);
    base::ScopedCFTypeRef<CFErrorRef> error;
    CFURLRef item_url_ref =
        LSSharedFileListItemCopyResolvedURL(item, 0, error.InitializeInto());
    if (!error && item_url_ref) {
      base::ScopedCFTypeRef<CFURLRef> item_url(item_url_ref);
      if (CFEqual(item_url, url)) {
        LSSharedFileListItemRemove(login_items, item);
        return;
      }
    }
  }
#pragma clang diagnostic pop
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
#if defined(MAS_BUILD)
  if (!platform_util::SetLoginItemEnabled(settings.open_at_login)) {
    LOG(ERROR) << "Unable to set login item enabled on sandboxed app.";
  }
#else
  if (settings.open_at_login) {
    base::mac::AddToLoginItems(settings.open_as_hidden);
  } else {
    RemoveFromLoginItems();
  }
#endif
}

std::string Browser::GetExecutableFileVersion() const {
  return GetApplicationVersion();
}

std::string Browser::GetExecutableFileProductName() const {
  return GetApplicationName();
}

int Browser::DockBounce(BounceType type) {
  return [[AtomApplication sharedApplication]
      requestUserAttention:static_cast<NSRequestUserAttentionType>(type)];
}

void Browser::DockCancelBounce(int request_id) {
  [[AtomApplication sharedApplication] cancelUserAttentionRequest:request_id];
}

void Browser::DockSetBadgeText(const std::string& label) {
  NSDockTile* tile = [[AtomApplication sharedApplication] dockTile];
  [tile setBadgeLabel:base::SysUTF8ToNSString(label)];
}

void Browser::DockDownloadFinished(const std::string& filePath) {
  [[NSDistributedNotificationCenter defaultCenter]
      postNotificationName:@"com.apple.DownloadFileFinished"
                    object:base::SysUTF8ToNSString(filePath)];
}

std::string Browser::DockGetBadgeText() {
  NSDockTile* tile = [[AtomApplication sharedApplication] dockTile];
  return base::SysNSStringToUTF8([tile badgeLabel]);
}

void Browser::DockHide() {
  // Transforming application state from UIElement to Foreground is an
  // asyncronous operation, and unfortunately there is currently no way to know
  // when it is finished.
  // So if we call DockHide => DockShow => DockHide => DockShow in a very short
  // time, we would triger a bug of macOS that, there would be multiple dock
  // icons of the app left in system.
  // To work around this, we make sure DockHide does nothing if it is called
  // immediately after DockShow. After some experiments, 1 second seems to be
  // a proper interval.
  if (!last_dock_show_.is_null() &&
      base::Time::Now() - last_dock_show_ < base::Seconds(1)) {
    return;
  }

  for (auto* const& window : WindowList::GetWindows())
    [window->GetNativeWindow().GetNativeNSWindow() setCanHide:NO];

  ProcessSerialNumber psn = {0, kCurrentProcess};
  TransformProcessType(&psn, kProcessTransformToUIElementApplication);
}

bool Browser::DockIsVisible() {
  // Because DockShow has a slight delay this may not be true immediately
  // after that call.
  return ([[NSRunningApplication currentApplication] activationPolicy] ==
          NSApplicationActivationPolicyRegular);
}

v8::Local<v8::Promise> Browser::DockShow(v8::Isolate* isolate) {
  last_dock_show_ = base::Time::Now();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  BOOL active = [[NSRunningApplication currentApplication] isActive];
  ProcessSerialNumber psn = {0, kCurrentProcess};
  if (active) {
    // Workaround buggy behavior of TransformProcessType.
    // http://stackoverflow.com/questions/7596643/
    NSArray* runningApps = [NSRunningApplication
        runningApplicationsWithBundleIdentifier:@"com.apple.dock"];
    for (NSRunningApplication* app in runningApps) {
      [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
      break;
    }
    __block gin_helper::Promise<void> p = std::move(promise);
    dispatch_time_t one_ms = dispatch_time(DISPATCH_TIME_NOW, USEC_PER_SEC);
    dispatch_after(one_ms, dispatch_get_main_queue(), ^{
      TransformProcessType(&psn, kProcessTransformToForegroundApplication);
      dispatch_time_t one_ms = dispatch_time(DISPATCH_TIME_NOW, USEC_PER_SEC);
      dispatch_after(one_ms, dispatch_get_main_queue(), ^{
        [[NSRunningApplication currentApplication]
            activateWithOptions:NSApplicationActivateIgnoringOtherApps];
        p.Resolve();
      });
    });
  } else {
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
    promise.Resolve();
  }
  return handle;
}

void Browser::DockSetMenu(ElectronMenuModel* model) {
  ElectronApplicationDelegate* delegate =
      (ElectronApplicationDelegate*)[NSApp delegate];
  [delegate setApplicationDockMenu:model];
}

void Browser::DockSetIcon(v8::Isolate* isolate, v8::Local<v8::Value> icon) {
  gfx::Image image;

  if (!icon->IsNull()) {
    api::NativeImage* native_image = nullptr;
    if (!api::NativeImage::TryConvertNativeImage(isolate, icon, &native_image))
      return;
    image = native_image->image();
  }

  [[AtomApplication sharedApplication]
      setApplicationIconImage:image.AsNSImage()];
}

void Browser::ShowAboutPanel() {
  NSDictionary* options = DictionaryValueToNSDictionary(about_panel_options_);

  // Credits must be a NSAttributedString instead of NSString
  NSString* credits = (NSString*)options[@"Credits"];
  if (credits != nil) {
    base::scoped_nsobject<NSMutableDictionary> mutable_options(
        [options mutableCopy]);
    base::scoped_nsobject<NSAttributedString> creditString(
        [[NSAttributedString alloc]
            initWithString:credits
                attributes:@{
                  NSForegroundColorAttributeName : [NSColor textColor]
                }]);

    [mutable_options setValue:creditString forKey:@"Credits"];
    options = [NSDictionary dictionaryWithDictionary:mutable_options];
  }

  [[AtomApplication sharedApplication]
      orderFrontStandardAboutPanelWithOptions:options];
}

void Browser::SetAboutPanelOptions(base::DictionaryValue options) {
  about_panel_options_.DictClear();

  for (const auto pair : options.DictItems()) {
    std::string key = std::string(pair.first);
    if (!key.empty() && pair.second.is_string()) {
      key[0] = base::ToUpperASCII(key[0]);
      auto val = std::make_unique<base::Value>(pair.second.Clone());
      about_panel_options_.Set(key, std::move(val));
    }
  }
}

void Browser::ShowEmojiPanel() {
  [[AtomApplication sharedApplication] orderFrontCharacterPalette:nil];
}

bool Browser::IsEmojiPanelSupported() {
  return true;
}

bool Browser::IsSecureKeyboardEntryEnabled() {
  return password_input_enabler_.get() != nullptr;
}

void Browser::SetSecureKeyboardEntryEnabled(bool enabled) {
  if (enabled) {
    password_input_enabler_ =
        std::make_unique<ui::ScopedPasswordInputEnabler>();
  } else {
    password_input_enabler_.reset();
  }
}

void Browser::RegisterForRemoteNotifications() {
  [[AtomApplication sharedApplication]
      registerForRemoteNotificationTypes:NSRemoteNotificationTypeBadge |
                                         NSRemoteNotificationTypeAlert |
                                         NSRemoteNotificationTypeSound];
}

void Browser::UnregisterForRemoteNotifications() {
  [[AtomApplication sharedApplication] unregisterForRemoteNotifications];
}

void Browser::DidRegisterForRemoteNotificationsWithDeviceToken(
    const std::string& token) {
  for (BrowserObserver& observer : observers_)
    observer.OnDidRegisterForRemoteNotificationsWithDeviceToken(token);
}

void Browser::DidFailToRegisterForRemoteNotificationsWithError(
    const std::string& error) {
  for (BrowserObserver& observer : observers_)
    observer.OnDidFailToRegisterForRemoteNotificationsWithError(error);
}

void Browser::DidReceiveRemoteNotification(
    const base::DictionaryValue& user_info) {
  for (BrowserObserver& observer : observers_)
    observer.OnDidReceiveRemoteNotification(user_info);
}

}  // namespace electron
