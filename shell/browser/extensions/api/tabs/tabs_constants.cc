// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/tabs/tabs_constants.h"

namespace extensions {
namespace tabs_constants {

const char kActiveKey[] = "active";
const char kAllFramesKey[] = "allFrames";
const char kAlwaysOnTopKey[] = "alwaysOnTop";
const char kBypassCache[] = "bypassCache";
const char kCodeKey[] = "code";
const char kCurrentWindowKey[] = "currentWindow";
const char kFaviconUrlKey[] = "favIconUrl";
const char kFileKey[] = "file";
const char kFocusedKey[] = "focused";
const char kFormatKey[] = "format";
const char kFromIndexKey[] = "fromIndex";
const char kGroupIdKey[] = "groupId";
const char kHeightKey[] = "height";
const char kIdKey[] = "id";
const char kIncognitoKey[] = "incognito";
const char kIndexKey[] = "index";
const char kLastFocusedWindowKey[] = "lastFocusedWindow";
const char kLeftKey[] = "left";
const char kNewPositionKey[] = "newPosition";
const char kNewWindowIdKey[] = "newWindowId";
const char kOldPositionKey[] = "oldPosition";
const char kOldWindowIdKey[] = "oldWindowId";
const char kOpenerTabIdKey[] = "openerTabId";
const char kPinnedKey[] = "pinned";
const char kAudibleKey[] = "audible";
const char kDiscardedKey[] = "discarded";
const char kAutoDiscardableKey[] = "autoDiscardable";
const char kMutedKey[] = "muted";
const char kMutedInfoKey[] = "mutedInfo";
const char kQualityKey[] = "quality";
const char kHighlightedKey[] = "highlighted";
const char kRunAtKey[] = "runAt";
const char kSelectedKey[] = "selected";
const char kShowStateKey[] = "state";
const char kStatusKey[] = "status";
const char kTabIdKey[] = "tabId";
const char kTabIdsKey[] = "tabIds";
const char kTabsKey[] = "tabs";
const char kTitleKey[] = "title";
const char kToIndexKey[] = "toIndex";
const char kTopKey[] = "top";
const char kUrlKey[] = "url";
const char kPendingUrlKey[] = "pendingUrl";
const char kWindowClosing[] = "isWindowClosing";
const char kWidthKey[] = "width";
const char kWindowIdKey[] = "windowId";
const char kWindowTypeKey[] = "type";
const char kWindowTypeLongKey[] = "windowType";
const char kWindowTypesKey[] = "windowTypes";
const char kZoomSettingsMode[] = "mode";
const char kZoomSettingsScope[] = "scope";

const char kShowStateValueNormal[] = "normal";
const char kShowStateValueMinimized[] = "minimized";
const char kShowStateValueMaximized[] = "maximized";
const char kShowStateValueFullscreen[] = "fullscreen";
const char kShowStateValueLockedFullscreen[] = "locked-fullscreen";

const char kWindowTypeValueNormal[] = "normal";
const char kWindowTypeValuePopup[] = "popup";
const char kWindowTypeValueApp[] = "app";
const char kWindowTypeValueDevTools[] = "devtools";

const char kCannotZoomDisabledTabError[] =
    "Cannot zoom a tab in disabled "
    "mode.";
const char kCanOnlyMoveTabsWithinNormalWindowsError[] =
    "Tabs can only be "
    "moved to and from normal windows.";
const char kCanOnlyMoveTabsWithinSameProfileError[] =
    "Tabs can only be moved "
    "between windows in the same profile.";
const char kFrameNotFoundError[] = "No frame with id * in tab *.";
const char kNoCrashBrowserError[] = "I'm sorry. I'm afraid I can't do that.";
const char kNoCurrentWindowError[] = "No current window";
const char kNoLastFocusedWindowError[] = "No last-focused window";
const char kNoTabInBrowserWindowError[] = "There is no tab in browser window.";
const char kPerOriginOnlyInAutomaticError[] =
    "Can only set scope to "
    "\"per-origin\" in \"automatic\" mode.";
const char kWindowNotFoundError[] = "No window with id: *.";
const char kTabIndexNotFoundError[] = "No tab at index: *.";
const char kNotFoundNextPageError[] = "Cannot find a next page in history.";
const char kTabNotFoundError[] = "No tab with id: *.";
const char kCannotDiscardTab[] = "Cannot discard tab with id: *.";
const char kCannotFindTabToDiscard[] = "Cannot find a tab to discard.";
const char kTabStripNotEditableError[] =
    "Tabs cannot be edited right now (user may be dragging a tab).";
const char kNoSelectedTabError[] = "No selected tab";
const char kNoHighlightedTabError[] = "No highlighted tab";
const char kIncognitoModeIsDisabled[] = "Incognito mode is disabled.";
const char kIncognitoModeIsForced[] =
    "Incognito mode is forced. "
    "Cannot open normal windows.";
const char kURLsNotAllowedInIncognitoError[] =
    "Cannot open URL \"*\" "
    "in an incognito window.";
const char kInvalidUrlError[] = "Invalid url: \"*\".";
const char kNotImplementedError[] = "This call is not yet implemented";
const char kSupportedInWindowsOnlyError[] = "Supported in Windows only";
const char kInvalidWindowTypeError[] = "Invalid value for type";
const char kInvalidWindowStateError[] = "Invalid value for state";
const char kScreenshotsDisabled[] = "Taking screenshots has been disabled";
const char kCannotUpdateMuteCaptured[] =
    "Cannot update mute state for tab *, tab has audio or video currently "
    "being captured";
const char kCannotDetermineLanguageOfUnloadedTab[] =
    "Cannot determine language: tab not loaded";
const char kMissingLockWindowFullscreenPrivatePermission[] =
    "Cannot lock window to fullscreen or close a locked fullscreen window "
    "without lockWindowFullscreenPrivate manifest permission";
const char kJavaScriptUrlsNotAllowedInTabsUpdate[] =
    "JavaScript URLs are not allowed in chrome.tabs.update. Use "
    "chrome.tabs.executeScript instead.";
const char kBrowserWindowNotAllowed[] = "Browser windows not allowed.";
const char kLockedFullscreenModeNewTabError[] =
    "You cannot create new tabs while in locked fullscreen mode.";
const char kGroupParamsError[] =
    "Cannot specify 'createProperties' along with a 'groupId'.";

}  // namespace tabs_constants
}  // namespace extensions