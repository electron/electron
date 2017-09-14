// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/mac/atom_application.h"

#include "atom/browser/mac/dict_util.h"
#include "atom/browser/browser.h"
#include "base/auto_reset.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_accessibility_state.h"

namespace {

inline void dispatch_sync_main(dispatch_block_t block) {
  if ([NSThread isMainThread])
    block();
  else
    dispatch_sync(dispatch_get_main_queue(), block);
}

}  // namespace

@implementation AtomApplication

+ (AtomApplication*)sharedApplication {
  return (AtomApplication*)[super sharedApplication];
}

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)sendEvent:(NSEvent*)event {
  base::AutoReset<BOOL> scoper(&handlingSendEvent_, YES);
  [super sendEvent:event];
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL {
  currentActivity_ = base::scoped_nsobject<NSUserActivity>(
      [[NSUserActivity alloc] initWithActivityType:type]);
  [currentActivity_ setUserInfo:userInfo];
  [currentActivity_ setWebpageURL:webpageURL];
  [currentActivity_ setDelegate:self];
  [currentActivity_ becomeCurrent];
  [currentActivity_ setNeedsSave:YES];
}

- (NSUserActivity*)getCurrentActivity {
  return currentActivity_.get();
}

- (void)invalidateCurrentActivity {
  if (currentActivity_) {
    [currentActivity_ invalidate];
    currentActivity_.reset();
  }
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

- (void)userActivityWillSave:(NSUserActivity *)userActivity {
  __block BOOL shouldWait = NO;
  dispatch_sync_main(^{
    std::string activity_type(base::SysNSStringToUTF8(userActivity.activityType));
    std::unique_ptr<base::DictionaryValue> user_info =
      atom::NSDictionaryToDictionaryValue(userActivity.userInfo);

    atom::Browser* browser = atom::Browser::Get();
    shouldWait = browser->UpdateUserActivityState(activity_type, *user_info) ? YES : NO;    
  });

  if (shouldWait) {
    [handoffLock_ lock];
    updateReceived_ = NO;
    while (!updateReceived_) {
      BOOL isSignaled = [handoffLock_ waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
      if (!isSignaled) break;
    }
    [handoffLock_ unlock];
  }

  [userActivity setNeedsSave:YES];
}

- (void)userActivityWasContinued:(NSUserActivity *)userActivity {
  dispatch_async(dispatch_get_main_queue(), ^{
    std::string activity_type(base::SysNSStringToUTF8(userActivity.activityType));
    std::unique_ptr<base::DictionaryValue> user_info =
    atom::NSDictionaryToDictionaryValue(userActivity.userInfo);

    atom::Browser* browser = atom::Browser::Get();
    browser->UserActivityWasContinued(activity_type, *user_info);
  });
  [userActivity setNeedsSave:YES];
}

- (void)awakeFromNib {
  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:self
          andSelector:@selector(handleURLEvent:withReplyEvent:)
        forEventClass:kInternetEventClass
           andEventID:kAEGetURL];

  handoffLock_ = [NSCondition new];
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  NSString* url = [
      [event paramDescriptorForKeyword:keyDirectObject] stringValue];
  atom::Browser::Get()->OpenURL(base::SysNSStringToUTF8(url));
}

- (bool)voiceOverEnabled {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  [defaults addSuiteNamed:@"com.apple.universalaccess"];
  [defaults synchronize];

  return [defaults boolForKey:@"voiceOverOnOffKey"];
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute {
  // Undocumented attribute that VoiceOver happens to set while running.
  // Chromium uses this too, even though it's not exactly right.
  if ([attribute isEqualToString:@"AXEnhancedUserInterface"]) {
    bool enableAccessibility = ([self voiceOverEnabled] && [value boolValue]);
    [self updateAccessibilityEnabled:enableAccessibility];
  }
  else if ([attribute isEqualToString:@"AXManualAccessibility"]) {
    [self updateAccessibilityEnabled:[value boolValue]];
  }
  return [super accessibilitySetValue:value forAttribute:attribute];
}

- (void)updateAccessibilityEnabled:(BOOL)enabled {
  auto ax_state = content::BrowserAccessibilityState::GetInstance();

  if (enabled) {
    ax_state->OnScreenReaderDetected();
  } else {
    ax_state->DisableAccessibility();
  }

  atom::Browser::Get()->OnAccessibilitySupportChanged();
}

- (void)orderFrontStandardAboutPanel:(id)sender {
  atom::Browser::Get()->ShowAboutPanel();
}

@end
