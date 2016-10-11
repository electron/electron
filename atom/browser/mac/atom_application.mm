// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/mac/atom_application.h"

#include "atom/browser/browser.h"
#include "base/auto_reset.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_accessibility_state.h"

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
  [currentActivity_ becomeCurrent];
}

- (NSUserActivity*)getCurrentActivity {
  return currentActivity_.get();
}

- (void)awakeFromNib {
  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:self
          andSelector:@selector(handleURLEvent:withReplyEvent:)
        forEventClass:kInternetEventClass
           andEventID:kAEGetURL];
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
