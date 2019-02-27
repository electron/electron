// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/scoped_sending_event.h"

#import <AVFoundation/AVFoundation.h>

// Forward Declare Appearance APIs
@interface NSApplication (HighSierraSDK)
@property(copy, readonly)
    NSAppearance* effectiveAppearance API_AVAILABLE(macosx(10.14));
@property(copy, readonly) NSAppearance* appearance API_AVAILABLE(macosx(10.14));
- (void)setAppearance:(NSAppearance*)appearance API_AVAILABLE(macosx(10.14));
@end

// forward declare Access APIs
typedef NSString* AVMediaType NS_EXTENSIBLE_STRING_ENUM;

AVF_EXPORT AVMediaType const AVMediaTypeVideo;
AVF_EXPORT AVMediaType const AVMediaTypeAudio;

typedef NS_ENUM(NSInteger, AVAuthorizationStatusMac) {
  AVAuthorizationStatusNotDeterminedMac = 0,
  AVAuthorizationStatusRestrictedMac = 1,
  AVAuthorizationStatusDeniedMac = 2,
  AVAuthorizationStatusAuthorizedMac = 3,
};

@interface AVCaptureDevice (MojaveSDK)
+ (void)requestAccessForMediaType:(AVMediaType)mediaType
                completionHandler:(void (^)(BOOL granted))handler
    API_AVAILABLE(macosx(10.14));
+ (AVAuthorizationStatusMac)authorizationStatusForMediaType:
    (AVMediaType)mediaType API_AVAILABLE(macosx(10.14));
@end

@interface NSColor (MojaveSDK)
@property(class, strong, readonly)
    NSColor* controlAccentColor API_AVAILABLE(macosx(10.14));

// macOS system colors
@property(class, strong, readonly)
    NSColor* systemBlueColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemBrownColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemGrayColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemGreenColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemOrangeColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemPinkColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemPurpleColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemRedColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* systemYellowColor API_AVAILABLE(macosx(10.10));

// misc dynamic colors declarations
@property(class, strong, readonly)
    NSColor* linkColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* placeholderTextColor API_AVAILABLE(macosx(10.10));
@property(class, strong, readonly)
    NSColor* findHighlightColor API_AVAILABLE(macosx(10.13));
@property(class, strong, readonly)
    NSColor* separatorColor API_AVAILABLE(macosx(10.14));
@property(class, strong, readonly)
    NSColor* selectedContentBackgroundColor API_AVAILABLE(macosx(10.14));
@property(class, strong, readonly)
    NSColor* unemphasizedSelectedContentBackgroundColor API_AVAILABLE(
        macosx(10.14));
@property(class, strong, readonly)
    NSColor* unemphasizedSelectedTextBackgroundColor API_AVAILABLE(macosx(10.14)
    );
@property(class, strong, readonly)
    NSColor* unemphasizedSelectedTextColor API_AVAILABLE(macosx(10.14));
@end

@interface AtomApplication : NSApplication <CrAppProtocol,
                                            CrAppControlProtocol,
                                            NSUserActivityDelegate> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_;
  NSCondition* handoffLock_;
  BOOL updateReceived_;
  base::Callback<bool()> shouldShutdown_;
}

+ (AtomApplication*)sharedApplication;

- (void)setShutdownHandler:(base::Callback<bool()>)handler;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;
- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL;
- (void)invalidateCurrentActivity;
- (void)updateCurrentActivity:(NSString*)type
                 withUserInfo:(NSDictionary*)userInfo;

@end
