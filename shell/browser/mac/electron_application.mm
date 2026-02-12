// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "shell/browser/mac/electron_application.h"

#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/mac/mac_util.h"
#include "base/observer_list.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/native_event_processor_mac.h"
#include "content/public/browser/native_event_processor_observer_mac.h"
#include "content/public/browser/scoped_accessibility_mode.h"
#include "content/public/common/content_features.h"
#include "shell/browser/browser.h"
#include "shell/browser/mac/dict_util.h"
#import "shell/browser/mac/electron_application_delegate.h"
#include "ui/accessibility/ax_mode.h"

namespace {

inline void dispatch_sync_main(dispatch_block_t block) {
  if ([NSThread isMainThread])
    block();
  else
    dispatch_sync(dispatch_get_main_queue(), block);
}

}  // namespace

@interface AtomApplication () <NativeEventProcessor> {
  int _AXEnhancedUserInterfaceRequests;
  BOOL _voiceOverEnabled;
  BOOL _sonomaAccessibilityRefinementsAreActive;
  base::ObserverList<content::NativeEventProcessorObserver>::Unchecked
      observers_;
}
// Enables/disables screen reader support on changes to VoiceOver status.
- (void)voiceOverStateChanged:(BOOL)voiceOverEnabled;
@end

@implementation AtomApplication {
  std::unique_ptr<content::ScopedAccessibilityMode>
      _scoped_accessibility_mode_voiceover;
  std::unique_ptr<content::ScopedAccessibilityMode>
      _scoped_accessibility_mode_general;
}

+ (AtomApplication*)sharedApplication {
  return (AtomApplication*)[super sharedApplication];
}

- (void)finishLaunching {
  [super finishLaunching];

  _sonomaAccessibilityRefinementsAreActive =
      base::mac::MacOSVersion() >= 14'00'00 &&
      base::FeatureList::IsEnabled(
          features::kSonomaAccessibilityActivationRefinements);
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
  if ([keyPath isEqualToString:@"voiceOverEnabled"] &&
      context == content::BrowserAccessibilityState::GetInstance()) {
    NSNumber* newValueNumber = [change objectForKey:NSKeyValueChangeNewKey];
    DCHECK([newValueNumber isKindOfClass:[NSNumber class]]);

    if ([newValueNumber isKindOfClass:[NSNumber class]]) {
      [self voiceOverStateChanged:[newValueNumber boolValue]];
    }

    return;
  }

  [super observeValueForKeyPath:keyPath
                       ofObject:object
                         change:change
                        context:context];
}

- (void)willPowerOff:(NSNotification*)notify {
  userStoppedShutdown_ = shouldShutdown_ && !shouldShutdown_.Run();
}

- (void)terminate:(id)sender {
  // User will call Quit later.
  if (userStoppedShutdown_)
    return;

  // We simply try to close the browser, which in turn will try to close the
  // windows. Termination can proceed if all windows are closed or window close
  // can be cancelled which will abort termination.
  electron::Browser::Get()->Quit();
}

- (void)setShutdownHandler:(base::RepeatingCallback<bool()>)handler {
  shouldShutdown_ = std::move(handler);
}

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)sendEvent:(NSEvent*)event {
  base::AutoReset<BOOL> scoper(&handlingSendEvent_, YES);
  content::ScopedNotifyNativeEventProcessorObserver scopedObserverNotifier(
      &observers_, event);
  [super sendEvent:event];
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL {
  currentActivity_ = [[NSUserActivity alloc] initWithActivityType:type];
  [currentActivity_ setUserInfo:userInfo];
  [currentActivity_ setWebpageURL:webpageURL];
  [currentActivity_ setDelegate:self];
  [currentActivity_ becomeCurrent];
  [currentActivity_ setNeedsSave:YES];
}

- (NSUserActivity*)getCurrentActivity {
  return currentActivity_;
}

- (void)invalidateCurrentActivity {
  if (currentActivity_) {
    [currentActivity_ invalidate];
    currentActivity_ = nil;
  }
}

- (void)resignCurrentActivity {
  if (currentActivity_)
    [currentActivity_ resignCurrent];
}

- (void)updateCurrentActivity:(NSString*)type
                 withUserInfo:(NSDictionary*)userInfo {
  if (currentActivity_) {
    [currentActivity_ addUserInfoEntriesFromDictionary:userInfo];
  }

  [handoffLock_ lock];
  updateReceived_ = YES;
  [handoffLock_ signal];
  [handoffLock_ unlock];
}

- (void)userActivityWillSave:(NSUserActivity*)userActivity {
  __block BOOL shouldWait = NO;
  dispatch_sync_main(^{
    std::string activity_type(
        base::SysNSStringToUTF8(userActivity.activityType));
    base::DictValue user_info =
        electron::NSDictionaryToValue(userActivity.userInfo);

    electron::Browser* browser = electron::Browser::Get();
    shouldWait =
        browser->UpdateUserActivityState(activity_type, std::move(user_info))
            ? YES
            : NO;
  });

  if (shouldWait) {
    [handoffLock_ lock];
    updateReceived_ = NO;
    while (!updateReceived_) {
      BOOL isSignaled =
          [handoffLock_ waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
      if (!isSignaled)
        break;
    }
    [handoffLock_ unlock];
  }

  [userActivity setNeedsSave:YES];
}

- (void)userActivityWasContinued:(NSUserActivity*)userActivity {
  dispatch_async(dispatch_get_main_queue(), ^{
    std::string activity_type(
        base::SysNSStringToUTF8(userActivity.activityType));
    base::DictValue user_info =
        electron::NSDictionaryToValue(userActivity.userInfo);

    electron::Browser* browser = electron::Browser::Get();
    browser->UserActivityWasContinued(activity_type, std::move(user_info));
  });
  [userActivity setNeedsSave:YES];
}

- (void)registerURLHandler {
  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:self
          andSelector:@selector(handleURLEvent:withReplyEvent:)
        forEventClass:kInternetEventClass
           andEventID:kAEGetURL];

  handoffLock_ = [NSCondition new];
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  NSString* url =
      [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
  electron::Browser::Get()->OpenURL(base::SysNSStringToUTF8(url));
}

// Returns the list of accessibility attributes that this object supports.
- (NSArray*)accessibilityAttributeNames {
  NSMutableArray* attributes =
      [[super accessibilityAttributeNames] mutableCopy];
  [attributes addObject:@"AXManualAccessibility"];
  return attributes;
}

// Returns whether or not the specified attribute can be set by the
// accessibility API via |accessibilitySetValue:forAttribute:|.
- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute {
  bool is_manual_ax = [attribute isEqualToString:@"AXManualAccessibility"];
  return is_manual_ax || [super accessibilityIsAttributeSettable:attribute];
}

// Returns the accessibility value for the given attribute.  If the value isn't
// supported this will return nil.
- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqualToString:@"AXManualAccessibility"]) {
    auto* ax_state = content::BrowserAccessibilityState::GetInstance();
    bool is_accessible_browser =
        ax_state->GetAccessibilityMode() == ui::kAXModeComplete;
    return [NSNumber numberWithBool:is_accessible_browser];
  }

  return [super accessibilityAttributeValue:attribute];
}

// Sets the value for an accessibility attribute via the accessibility API.
// AXEnhancedUserInterface is an undocumented attribute that screen reader
// related functionality sets when running, and AXManualAccessibility is an
// attribute Electron specifically allows third-party apps to use to enable
// a11y features in Electron.
- (void)accessibilitySetValue:(id)value forAttribute:(NSString*)attribute {
  bool is_manual_ax = [attribute isEqualToString:@"AXManualAccessibility"];
  if ([attribute isEqualToString:@"AXEnhancedUserInterface"] || is_manual_ax) {
    [self enableScreenReaderCompleteModeAfterDelay:[value boolValue]];
    electron::Browser::Get()->OnAccessibilitySupportChanged();

    // Don't call the superclass function for AXManualAccessibility,
    // as it will log an AXError and make it appear as though the attribute
    // failed to take effect.
    if (is_manual_ax)
      return;
  }

  return [super accessibilitySetValue:value forAttribute:attribute];
}

// FROM:
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/chrome_browser_application_mac.mm;l=549;drc=4341cc4e529444bd201ad3aeeed0ec561e04103f
- (NSAccessibilityRole)accessibilityRole {
  // For non-VoiceOver assistive technology (AT), such as Voice Control, Apple
  // recommends turning on a11y when an AT accesses the 'accessibilityRole'
  // property. This function is accessed frequently, so we only change the
  // accessibility state when accessibility is already disabled.
  if (!_scoped_accessibility_mode_general &&
      !_scoped_accessibility_mode_voiceover) {
    ui::AXMode target_mode = _sonomaAccessibilityRefinementsAreActive
                                 ? ui::AXMode::kNativeAPIs
                                 : ui::kAXModeBasic;
    _scoped_accessibility_mode_general =
        content::BrowserAccessibilityState::GetInstance()
            ->CreateScopedModeForProcess(target_mode |
                                         ui::AXMode::kFromPlatform);
  }

  return [super accessibilityRole];
}

- (void)enableScreenReaderCompleteMode:(BOOL)enable {
  if (enable) {
    if (!_scoped_accessibility_mode_voiceover) {
      _scoped_accessibility_mode_voiceover =
          content::BrowserAccessibilityState::GetInstance()
              ->CreateScopedModeForProcess(ui::kAXModeComplete |
                                           ui::AXMode::kFromPlatform |
                                           ui::AXMode::kScreenReader);
    }
  } else {
    _scoped_accessibility_mode_voiceover.reset();
  }
}

// We need to call enableScreenReaderCompleteMode:YES from performSelector:...
// but there's no way to supply a BOOL as a parameter, so we have this
// explicit enable... helper method.
- (void)enableScreenReaderCompleteMode {
  _AXEnhancedUserInterfaceRequests = 0;
  [self enableScreenReaderCompleteMode:YES];
}

- (void)voiceOverStateChanged:(BOOL)voiceOverEnabled {
  _voiceOverEnabled = voiceOverEnabled;

  [self enableScreenReaderCompleteMode:voiceOverEnabled];
}

// Enables or disables screen reader support for non-VoiceOver assistive
// technology (AT), possibly after a delay.
//
// Now that we directly monitor VoiceOver status, we no longer watch for
// changes to AXEnhancedUserInterface for that signal from VO. However, other
// AT can set a value for AXEnhancedUserInterface, so we can't ignore it.
// Unfortunately, as of macOS Sonoma, we sometimes see spurious changes to
// AXEnhancedUserInterface (quick on and off). We debounce by waiting for these
// changes to settle down before updating the screen reader state.
- (void)enableScreenReaderCompleteModeAfterDelay:(BOOL)enable {
  // If VoiceOver is already explicitly enabled, ignore requests from other AT.
  if (_voiceOverEnabled) {
    return;
  }

  // If this is a request to disable screen reader support, and we haven't seen
  // a corresponding enable request, go ahead and disable.
  if (!enable && _AXEnhancedUserInterfaceRequests == 0) {
    [self enableScreenReaderCompleteMode:NO];
    return;
  }

  // Use a counter to track requests for changes to the screen reader state.
  if (enable) {
    _AXEnhancedUserInterfaceRequests++;
  } else {
    _AXEnhancedUserInterfaceRequests--;
  }

  DCHECK_GE(_AXEnhancedUserInterfaceRequests, 0);

  // _AXEnhancedUserInterfaceRequests > 0 means we want to enable screen
  // reader support, but we'll delay that action until there are no more state
  // change requests within a two-second window. Cancel any pending
  // performSelector:..., and schedule a new one to restart the countdown.
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector
                                           (enableScreenReaderCompleteMode)
                                             object:nil];

  if (_AXEnhancedUserInterfaceRequests > 0) {
    const float kTwoSecondDelay = 2.0;
    [self performSelector:@selector(enableScreenReaderCompleteMode)
               withObject:nil
               afterDelay:kTwoSecondDelay];
  }
}

- (void)orderFrontStandardAboutPanel:(id)sender {
  electron::Browser::Get()->ShowAboutPanel();
}

- (void)addNativeEventProcessorObserver:
    (content::NativeEventProcessorObserver*)observer {
  observers_.AddObserver(observer);
}

- (void)removeNativeEventProcessorObserver:
    (content::NativeEventProcessorObserver*)observer {
  observers_.RemoveObserver(observer);
}

@end
