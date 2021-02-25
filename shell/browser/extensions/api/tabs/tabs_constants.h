// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants used for the Tabs API and the Windows API.

#ifndef SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_CONSTANTS_H_
#define SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_CONSTANTS_H_

namespace extensions {
namespace tabs_constants {

// Keys used in serializing tab data & events.
extern const char kActiveKey[];
extern const char kAllFramesKey[];
extern const char kAlwaysOnTopKey[];
extern const char kBypassCache[];
extern const char kCodeKey[];
extern const char kCurrentWindowKey[];
extern const char kFaviconUrlKey[];
extern const char kFileKey[];
extern const char kFocusedKey[];
extern const char kFormatKey[];
extern const char kFromIndexKey[];
extern const char kGroupIdKey[];
extern const char kHeightKey[];
extern const char kHighlightedKey[];
extern const char kIdKey[];
extern const char kIncognitoKey[];
extern const char kIndexKey[];
extern const char kLastFocusedWindowKey[];
extern const char kLeftKey[];
extern const char kNewPositionKey[];
extern const char kNewWindowIdKey[];
extern const char kOldPositionKey[];
extern const char kOldWindowIdKey[];
extern const char kOpenerTabIdKey[];
extern const char kPinnedKey[];
extern const char kAudibleKey[];
extern const char kDiscardedKey[];
extern const char kAutoDiscardableKey[];
extern const char kMutedKey[];
extern const char kMutedInfoKey[];
extern const char kQualityKey[];
extern const char kRunAtKey[];
extern const char kSelectedKey[];
extern const char kShowStateKey[];
extern const char kStatusKey[];
extern const char kTabIdKey[];
extern const char kTabIdsKey[];
extern const char kTabsKey[];
extern const char kTitleKey[];
extern const char kToIndexKey[];
extern const char kTopKey[];
extern const char kUrlKey[];
extern const char kPendingUrlKey[];
extern const char kWidthKey[];
extern const char kWindowClosing[];
extern const char kWindowIdKey[];
extern const char kWindowTypeKey[];
extern const char kWindowTypeLongKey[];
extern const char kWindowTypesKey[];
extern const char kZoomSettingsMode[];
extern const char kZoomSettingsScope[];

// Value consts.
extern const char kCanOnlyMoveTabsWithinNormalWindowsError[];
extern const char kCanOnlyMoveTabsWithinSameProfileError[];
extern const char kShowStateValueNormal[];
extern const char kShowStateValueMinimized[];
extern const char kShowStateValueMaximized[];
extern const char kShowStateValueFullscreen[];
extern const char kShowStateValueLockedFullscreen[];
extern const char kWindowTypeValueNormal[];
extern const char kWindowTypeValuePopup[];
extern const char kWindowTypeValueApp[];
extern const char kWindowTypeValueDevTools[];

// Error messages.
extern const char kCannotZoomDisabledTabError[];
extern const char kFrameNotFoundError[];
extern const char kNoCrashBrowserError[];
extern const char kNoCurrentWindowError[];
extern const char kNoLastFocusedWindowError[];
extern const char kNoTabInBrowserWindowError[];
extern const char kPerOriginOnlyInAutomaticError[];
extern const char kWindowNotFoundError[];
extern const char kTabIndexNotFoundError[];
extern const char kNotFoundNextPageError[];
extern const char kTabNotFoundError[];
extern const char kCannotDiscardTab[];
extern const char kCannotFindTabToDiscard[];
extern const char kTabStripNotEditableError[];
extern const char kNoHighlightedTabError[];
extern const char kNoSelectedTabError[];
extern const char kIncognitoModeIsDisabled[];
extern const char kIncognitoModeIsForced[];
extern const char kURLsNotAllowedInIncognitoError[];
extern const char kInvalidUrlError[];
extern const char kNotImplementedError[];
extern const char kSupportedInWindowsOnlyError[];
extern const char kInvalidWindowTypeError[];
extern const char kInvalidWindowStateError[];
extern const char kScreenshotsDisabled[];
extern const char kCannotUpdateMuteCaptured[];
extern const char kCannotDetermineLanguageOfUnloadedTab[];
extern const char kMissingLockWindowFullscreenPrivatePermission[];
extern const char kJavaScriptUrlsNotAllowedInTabsUpdate[];
extern const char kBrowserWindowNotAllowed[];
extern const char kLockedFullscreenModeNewTabError[];
extern const char kGroupParamsError[];

}  // namespace tabs_constants
}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_CONSTANTS_H_