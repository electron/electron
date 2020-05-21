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
#include "shell/browser/mac/dict_util.h"
#include "shell/browser/mac/electron_application.h"
#include "shell/browser/mac/electron_application_delegate.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/platform_util.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace electron {

void Browser::SetShutdownHandler(base::Callback<bool()> handler) {
  [[AtomApplication sharedApplication] setShutdownHandler:std::move(handler)];
}

void Browser::Focus(gin_helper::Arguments* args) {
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
                                            gin_helper::Arguments* args) {
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
                                         gin_helper::Arguments* args) {
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
                                      gin_helper::Arguments* args) {
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

base::string16 Browser::GetApplicationNameForProtocol(const GURL& url) {
  NSURL* ns_url = [NSURL
      URLWithString:base::SysUTF8ToNSString(url.possibly_invalid_spec())];
  base::ScopedCFTypeRef<CFErrorRef> out_err;
  base::ScopedCFTypeRef<CFURLRef> openingApp(LSCopyDefaultApplicationURLForURL(
      (CFURLRef)ns_url, kLSRolesAll, out_err.InitializeInto()));
  if (out_err) {
    // likely kLSApplicationNotFoundErr
    return base::string16();
  }
  NSString* appPath = [base::mac::CFToNSCast(openingApp.get()) path];
  NSString* appDisplayName =
      [[NSFileManager defaultManager] displayNameAtPath:appPath];
  return base::SysNSStringToUTF16(appDisplayName);
}

void Browser::SetAppUserModelID(const base::string16& name) {}

bool Browser::SetBadgeCount(int count) {
  DockSetBadgeText(count != 0 ? base::NumberToString(count) : "");
  badge_count_ = count;
  return true;
}

void Browser::SetUserActivity(const std::string& type,
                              base::DictionaryValue user_info,
                              gin_helper::Arguments* args) {
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
                                   base::DictionaryValue user_info) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnContinueUserActivity(&prevent_default, type, user_info);
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

void Browser::DockSetIcon(const gfx::Image& image) {
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
  about_panel_options_.Clear();

  for (auto& pair : options) {
    std::string& key = pair.first;
    if (!key.empty() && pair.second->is_string()) {
      key[0] = base::ToUpperASCII(key[0]);
      about_panel_options_.Set(key, std::move(pair.second));
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

}  // namespace electron
