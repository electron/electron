// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_NOTIFICATION_TYPES_H_
#define CHROME_BROWSER_CHROME_NOTIFICATION_TYPES_H_

#include "build/build_config.h"

#if defined(ENABLE_EXTENSIONS)
#include "extensions/browser/notification_types.h"
#else
#include "content/public/browser/notification_types.h"
#endif

#if defined(ENABLE_EXTENSIONS)
#define PREVIOUS_END extensions::NOTIFICATION_EXTENSIONS_END
#else
#define PREVIOUS_END content::NOTIFICATION_CONTENT_END
#endif

namespace chrome {

// NotificationService &c. are deprecated (https://crbug.com/268984).
// Don't add any new notification types, and migrate existing uses of the
// notification types below to observers.
enum NotificationType {
  NOTIFICATION_CHROME_START = PREVIOUS_END,

  // Browser-window ----------------------------------------------------------

  // This message is sent after a window has been opened.  The source is a
  // Source<Browser> containing the affected Browser.  No details are
  // expected.
  NOTIFICATION_BROWSER_OPENED = NOTIFICATION_CHROME_START,

  // This message is sent soon after BROWSER_OPENED, and indicates that
  // the Browser's |window_| is now non-NULL. The source is a Source<Browser>
  // containing the affected Browser.  No details are expected.
  NOTIFICATION_BROWSER_WINDOW_READY,

  // This message is sent when a browser is closing. The source is a
  // Source<Browser> containing the affected Browser. No details are expected.
  // This is sent prior to BROWSER_CLOSED, and may be sent more than once for a
  // particular browser.
  NOTIFICATION_BROWSER_CLOSING,

  // This message is sent after a window has been closed.  The source is a
  // Source<Browser> containing the affected Browser.  No details are exptected.
  NOTIFICATION_BROWSER_CLOSED,

  // This message is sent when closing a browser has been cancelled, either by
  // the user cancelling a beforeunload dialog, or IsClosingPermitted()
  // disallowing closing. This notification implies that no BROWSER_CLOSING or
  // BROWSER_CLOSED notification will be sent.
  // The source is a Source<Browser> containing the affected browser. No details
  // are expected.
  NOTIFICATION_BROWSER_CLOSE_CANCELLED,

  // Sent when the language (English, French...) for a page has been detected.
  // The details Details<std::string> contain the ISO 639-1 language code and
  // the source is Source<WebContents>.
  NOTIFICATION_TAB_LANGUAGE_DETERMINED,

  // Sent when a page has been translated. The source is the tab for that page
  // (Source<WebContents>) and the details are the language the page was
  // originally in and the language it was translated to
  // (std::pair<std::string, std::string>).
  NOTIFICATION_PAGE_TRANSLATED,

  // The user has changed the browser theme. The source is a
  // Source<ThemeService>. There are no details.
  NOTIFICATION_BROWSER_THEME_CHANGED,

#if defined(USE_AURA)
  // The user has changed the fling curve configuration.
  // Source<GesturePrefsObserver>. There are no details.
  NOTIFICATION_BROWSER_FLING_CURVE_PARAMETERS_CHANGED,
#endif  // defined(USE_AURA)

  // Sent when the renderer returns focus to the browser, as part of focus
  // traversal. The source is the browser, there are no details.
  NOTIFICATION_FOCUS_RETURNED_TO_BROWSER,

  // A new tab is created from an existing tab to serve as a target of a
  // navigation that is about to happen. The source will be a Source<Profile>
  // corresponding to the profile in which the new tab will live.  Details in
  // the form of a RetargetingDetails object are provided.
  NOTIFICATION_RETARGETING,

  // Application-wide ----------------------------------------------------------

  // This message is sent when the application is terminating (the last
  // browser window has shutdown as part of an explicit user-initiated exit,
  // or the user closed the last browser window on Windows/Linux and there are
  // no BackgroundContents keeping the browser running). No source or details
  // are passed.
  NOTIFICATION_APP_TERMINATING,

#if defined(OS_MACOSX)
  // This notification is sent when the app has no key window, such as when
  // all windows are closed but the app is still active. No source or details
  // are provided.
  NOTIFICATION_NO_KEY_WINDOW,
#endif

  // This is sent when the user has chosen to exit the app, but before any
  // browsers have closed. This is sent if the user chooses to exit (via exit
  // menu item or keyboard shortcut) or to restart the process (such as in flags
  // page), not if Chrome exits by some other means (such as the user closing
  // the last window). No source or details are passed.
  //
  // Note that receiving this notification does not necessarily mean the process
  // will exit because the shutdown process can be cancelled by an unload
  // handler.  Use APP_TERMINATING for such needs.
  NOTIFICATION_CLOSE_ALL_BROWSERS_REQUEST,

  // This message is sent when a new InfoBar has been added to an
  // InfoBarService.  The source is a Source<InfoBarService> with a pointer to
  // the InfoBarService the InfoBar was added to.  The details is a
  // Details<InfoBar::AddedDetails>.
  NOTIFICATION_TAB_CONTENTS_INFOBAR_ADDED,

  // This message is sent when an InfoBar is about to be removed from an
  // InfoBarService.  The source is a Source<InfoBarService> with a pointer to
  // the InfoBarService the InfoBar was removed from.  The details is a
  // Details<InfoBar::RemovedDetails>.
  NOTIFICATION_TAB_CONTENTS_INFOBAR_REMOVED,

#if defined(ENABLE_EXTENSIONS)
  // This notification is sent when extensions::TabHelper::SetExtensionApp is
  // invoked. The source is the extensions::TabHelper SetExtensionApp was
  // invoked on.
  NOTIFICATION_TAB_CONTENTS_APPLICATION_EXTENSION_CHANGED,
#endif

  // Tabs --------------------------------------------------------------------

  // Sent when a tab is added to a WebContentsDelegate. The source is the
  // WebContentsDelegate and the details is the added WebContents.
  NOTIFICATION_TAB_ADDED,

  // This notification is sent after a tab has been appended to the tab_strip.
  // The source is a Source<WebContents> of the tab being added. There
  // are no details.
  NOTIFICATION_TAB_PARENTED,

  // This message is sent before a tab has been closed.  The source is a
  // Source<NavigationController> with a pointer to the controller for the
  // closed tab.  No details are expected.
  //
  // See also content::NOTIFICATION_WEB_CONTENTS_DESTROYED, which is sent when
  // the WebContents containing the NavigationController is destroyed.
  NOTIFICATION_TAB_CLOSING,

  // Stuff inside the tabs ---------------------------------------------------

  // This notification is sent when the result of a find-in-page search is
  // available with the browser process. The source is a Source<WebContents>.
  // Details encompass a FindNotificationDetail object that tells whether the
  // match was found or not found.
  NOTIFICATION_FIND_RESULT_AVAILABLE,

  // BackgroundContents ------------------------------------------------------

  // A new background contents was opened by script. The source is the parent
  // profile and the details are BackgroundContentsOpenedDetails.
  NOTIFICATION_BACKGROUND_CONTENTS_OPENED,

  // The background contents navigated to a new location. The source is the
  // parent Profile, and the details are the BackgroundContents that was
  // navigated.
  NOTIFICATION_BACKGROUND_CONTENTS_NAVIGATED,

  // The background contents were closed by someone invoking window.close()
  // or the parent application was uninstalled.
  // The source is the parent profile, and the details are the
  // BackgroundContents.
  NOTIFICATION_BACKGROUND_CONTENTS_CLOSED,

  // The background contents is being deleted. The source is the
  // parent Profile, and the details are the BackgroundContents being deleted.
  NOTIFICATION_BACKGROUND_CONTENTS_DELETED,

  // The background contents has crashed. The source is the parent Profile,
  // and the details are the BackgroundContents.
  NOTIFICATION_BACKGROUND_CONTENTS_TERMINATED,

  // The background contents associated with a hosted app has changed (either
  // a new background contents has been created, or an existing background
  // contents has closed). The source is the parent Profile, and the details
  // are the BackgroundContentsService.
  NOTIFICATION_BACKGROUND_CONTENTS_SERVICE_CHANGED,

  // Chrome has entered/exited background mode. The source is the
  // BackgroundModeManager and the details are a boolean value which is set to
  // true if Chrome is now in background mode.
  NOTIFICATION_BACKGROUND_MODE_CHANGED,

  // This is sent when a login prompt is shown.  The source is the
  // Source<NavigationController> for the tab in which the prompt is shown.
  // Details are a LoginNotificationDetails which provide the LoginHandler
  // that should be given authentication.
  NOTIFICATION_AUTH_NEEDED,

  // This is sent when authentication credentials have been supplied (either
  // by the user or by an automation service), but before we've actually
  // received another response from the server.  The source is the
  // Source<NavigationController> for the tab in which the prompt was shown.
  // Details are an AuthSuppliedLoginNotificationDetails which provide the
  // LoginHandler that should be given authentication as well as the supplied
  // username and password.
  NOTIFICATION_AUTH_SUPPLIED,

  // This is sent when an authentication request has been dismissed without
  // supplying credentials (either by the user or by an automation service).
  // The source is the Source<NavigationController> for the tab in which the
  // prompt was shown. Details are a LoginNotificationDetails which provide
  // the LoginHandler that should be cancelled.
  NOTIFICATION_AUTH_CANCELLED,

  // Profiles -----------------------------------------------------------------

  // Sent after a Profile has been created. This notification is sent both for
  // normal and OTR profiles.
  // The details are none and the source is the new profile.
  NOTIFICATION_PROFILE_CREATED,

  // Sent after a Profile has been added to ProfileManager.
  // The details are none and the source is the new profile.
  NOTIFICATION_PROFILE_ADDED,

  // Use KeyedServiceShutdownNotifier instead this notification type (you did
  // read the comment at the top of the file, didn't you?).
  // Sent early in the process of destroying a Profile, at the time a user
  // initiates the deletion of a profile versus the much later time when the
  // profile object is actually destroyed (use NOTIFICATION_PROFILE_DESTROYED).
  // The details are none and the source is a Profile*.
  NOTIFICATION_PROFILE_DESTRUCTION_STARTED,

  // Use KeyedServiceShutdownNotifier instead this notification type (you did
  // read the comment at the top of the file, didn't you?).
  // Sent before a Profile is destroyed. This notification is sent both for
  // normal and OTR profiles.
  // The details are none and the source is a Profile*.
  NOTIFICATION_PROFILE_DESTROYED,

  // Sent after the URLRequestContextGetter for a Profile has been initialized.
  // The details are none and the source is a Profile*.
  NOTIFICATION_PROFILE_URL_REQUEST_CONTEXT_GETTER_INITIALIZED,

  // Non-history storage services --------------------------------------------

  // The state of a web resource has been changed. A resource may have been
  // added, removed, or altered. Source is WebResourceService, and the
  // details are NoDetails.
  NOTIFICATION_PROMO_RESOURCE_STATE_CHANGED,

  // A safe browsing database update completed.  Source is the
  // SafeBrowsingService and the details are a bool indicating whether the
  // update was successful.
  NOTIFICATION_SAFE_BROWSING_UPDATE_COMPLETE,

  // Autocomplete ------------------------------------------------------------

  // Sent by the autocomplete controller when done.  The source is the
  // AutocompleteController, the details not used.
  NOTIFICATION_AUTOCOMPLETE_CONTROLLER_RESULT_READY,

  // This is sent from Instant when the omnibox focus state changes.
  NOTIFICATION_OMNIBOX_FOCUS_CHANGED,

  // Printing ----------------------------------------------------------------

  // Notification from PrintJob that an event occurred. It can be that a page
  // finished printing or that the print job failed. Details is
  // PrintJob::EventDetails. Source is a PrintJob.
  NOTIFICATION_PRINT_JOB_EVENT,

  // Sent when a PrintJob has been released.
  // Source is the WebContents that holds the print job.
  NOTIFICATION_PRINT_JOB_RELEASED,

  // Upgrade notifications ---------------------------------------------------

  // Sent when Chrome believes an update has been installed and available for
  // long enough with the user shutting down to let it take effect. See
  // upgrade_detector.cc for details on how long it waits. No details are
  // expected.
  NOTIFICATION_UPGRADE_RECOMMENDED,

  // Sent when a critical update has been installed. No details are expected.
  NOTIFICATION_CRITICAL_UPGRADE_INSTALLED,

  // Sent when the current install is outdated. No details are expected.
  NOTIFICATION_OUTDATED_INSTALL,

  // Sent when the current install is outdated and auto-update (AU) is disabled.
  // No details are expected.
  NOTIFICATION_OUTDATED_INSTALL_NO_AU,

  // Software incompatibility notifications ----------------------------------

  // Sent when Chrome has finished compiling the list of loaded modules (and
  // other modules of interest). No details are expected.
  NOTIFICATION_MODULE_LIST_ENUMERATED,

  // Sent when Chrome is done scanning the module list and when the user has
  // acknowledged the module incompatibility. No details are expected.
  NOTIFICATION_MODULE_INCOMPATIBILITY_BADGE_CHANGE,

  // Content Settings --------------------------------------------------------

  // Sent when the collect cookies dialog is shown. The source is a
  // TabSpecificContentSettings object, there are no details.
  NOTIFICATION_COLLECTED_COOKIES_SHOWN,

  // Sent when content settings change for a tab. The source is a
  // content::WebContents object, the details are None.
  NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED,

  // Sync --------------------------------------------------------------------

  // The session service has been saved.  This notification type is only sent
  // if there were new SessionService commands to save, and not for no-op save
  // operations.
  NOTIFICATION_SESSION_SERVICE_SAVED,

  // Cookies -----------------------------------------------------------------

#if defined(ENABLE_EXTENSIONS)
  // Sent when a cookie changes, for consumption by extensions. The source is a
  // Profile object, the details are a ChromeCookieDetails object.
  NOTIFICATION_COOKIE_CHANGED_FOR_EXTENSIONS,
#endif

  // Download Notifications --------------------------------------------------

  // Sent when a download is initiated. It is possible that the download will
  // not actually begin due to the DownloadRequestLimiter cancelling it
  // prematurely.
  // The source is the corresponding RenderViewHost. There are no details.
  NOTIFICATION_DOWNLOAD_INITIATED,

  // Misc --------------------------------------------------------------------

#if defined(OS_CHROMEOS)
  // Sent when a chromium os user logs in.
  // The details are a chromeos::User object.
  NOTIFICATION_LOGIN_USER_CHANGED,

  // Sent immediately after the logged-in user's profile is ready.
  // The details are a Profile object.
  NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,

  // Sent when the chromium session of a particular user is started.
  // If this is a new user on the machine this will not be sent until a profile
  // picture has been selected, unlike NOTIFICATION_LOGIN_USER_CHANGED which is
  // sent immediately after the user has logged in. This will be sent again if
  // the browser crashes and restarts.
  // The details are a chromeos::User object.
  NOTIFICATION_SESSION_STARTED,

  // Sent when user image is updated.
  NOTIFICATION_LOGIN_USER_IMAGE_CHANGED,

  // Sent by UserManager when a profile image download has been completed.
  NOTIFICATION_PROFILE_IMAGE_UPDATED,

  // Sent by UserManager when profile image download has failed or user has the
  // default profile image or no profile image at all. No details are expected.
  NOTIFICATION_PROFILE_IMAGE_UPDATE_FAILED,

  // Sent when a network error message is displayed on the WebUI login screen.
  // First paint event of this fires NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE.
  NOTIFICATION_LOGIN_NETWORK_ERROR_SHOWN,

  // Sent when the specific part of login/lock WebUI is considered to be
  // visible. That moment is tracked as the first paint event after one of the:
  // NOTIFICATION_LOGIN_NETWORK_ERROR_SHOWN
  //
  // Possible series of notifications:
  // 1. Boot into fresh OOBE
  //    NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE
  // 2. Boot into user pods list (normal boot). Same for lock screen.
  //    NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE
  // 3. Boot into GAIA sign in UI (user pods display disabled or no users):
  //    if no network is connected or flaky network
  //    (NOTIFICATION_LOGIN_NETWORK_ERROR_SHOWN +
  //     NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE)
  //    NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE
  // 4. Boot into retail mode
  //    NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE
  // 5. Boot into kiosk mode
  //    NOTIFICATION_KIOSK_APP_LAUNCHED
  NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,

  // Sent when proxy dialog is closed.
  NOTIFICATION_LOGIN_PROXY_CHANGED,

  // Send when kiosk auto-launch warning screen is visible.
  NOTIFICATION_KIOSK_AUTOLAUNCH_WARNING_VISIBLE,

  // Send when kiosk auto-launch warning screen had completed.
  NOTIFICATION_KIOSK_AUTOLAUNCH_WARNING_COMPLETED,

  // Send when enable consumer kiosk warning screen is visible.
  NOTIFICATION_KIOSK_ENABLE_WARNING_VISIBLE,

  // Send when consumer kiosk has been enabled.
  NOTIFICATION_KIOSK_ENABLED,

  // Send when enable consumer kiosk warning screen had completed.
  NOTIFICATION_KIOSK_ENABLE_WARNING_COMPLETED,

  // Sent when kiosk app list is loaded in UI.
  NOTIFICATION_KIOSK_APPS_LOADED,

  // Sent when a kiosk app is launched.
  NOTIFICATION_KIOSK_APP_LAUNCHED,

  // Sent when the user list has changed.
  NOTIFICATION_USER_LIST_CHANGED,

  // Sent when the screen lock state has changed. The source is
  // ScreenLocker and the details is a bool specifing that the
  // screen is locked. When details is a false, the source object
  // is being deleted, so the receiver shouldn't use the screen locker
  // object.
  NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,

  // Sent by DeviceSettingsService to indicate that the ownership status
  // changed. If you can, please use DeviceSettingsService::Observer instead.
  // Other singleton-based services can't use that because Observer
  // unregistration is impossible due to unpredictable deletion order.
  NOTIFICATION_OWNERSHIP_STATUS_CHANGED,
#endif

#if defined(TOOLKIT_VIEWS)
  // Sent when a bookmark's context menu is shown. Used to notify
  // tests that the context menu has been created and shown.
  NOTIFICATION_BOOKMARK_CONTEXT_MENU_SHOWN,

  // Notification that the nested loop using during tab dragging has returned.
  // Used for testing.
  NOTIFICATION_TAB_DRAG_LOOP_DONE,
#endif

  // Send when a context menu is shown. Used to notify tests that the context
  // menu has been created and shown.
  NOTIFICATION_RENDER_VIEW_CONTEXT_MENU_SHOWN,

  // Sent when the Instant Controller determines whether an Instant tab supports
  // the Instant API or not.
  NOTIFICATION_INSTANT_TAB_SUPPORT_DETERMINED,

  // Sent when the CaptivePortalService checks if we're behind a captive portal.
  // The Source is the Profile the CaptivePortalService belongs to, and the
  // Details are a Details<CaptivePortalService::CheckResults>.
  NOTIFICATION_CAPTIVE_PORTAL_CHECK_RESULT,

  // Sent when the applications in the NTP app launcher have been reordered.
  // The details, if not NoDetails, is the std::string ID of the extension that
  // was moved.
  NOTIFICATION_APP_LAUNCHER_REORDERED,

  // Sent when an app is installed and an NTP has been shown. Source is the
  // WebContents that was shown, and Details is the string ID of the extension
  // which was installed.
  NOTIFICATION_APP_INSTALLED_TO_NTP,

#if defined(USE_ASH)
  // Sent when wallpaper show animation has finished.
  NOTIFICATION_WALLPAPER_ANIMATION_FINISHED,

  // Sent when the Ash session has started. In its current incantation this is
  // generated when the metro app has connected to the browser IPC channel.
  // Used only on Windows.
  NOTIFICATION_ASH_SESSION_STARTED,

  // Sent when the Ash session ended. Currently this means the metro app exited.
  // Used only on Windows.
  NOTIFICATION_ASH_SESSION_ENDED,
#endif

  // Protocol Handler Registry -----------------------------------------------
  // Sent when a ProtocolHandlerRegistry is changed. The source is the profile.
  NOTIFICATION_PROTOCOL_HANDLER_REGISTRY_CHANGED,

  // Sent when the browser enters or exits fullscreen mode.
  NOTIFICATION_FULLSCREEN_CHANGED,

  // Sent when the FullscreenController changes, confirms, or denies mouse lock.
  // The source is the browser's FullscreenController, no details.
  NOTIFICATION_MOUSE_LOCK_CHANGED,

  // Sent by the PluginPrefs when there is a change of plugin enable/disable
  // status. The source is the profile.
  NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED,

  // Panels Notifications. The Panels are small browser windows near the bottom
  // of the screen.
  // Sent when all nonblocking bounds animations are finished across panels.
  // Used only in unit testing.
  NOTIFICATION_PANEL_BOUNDS_ANIMATIONS_FINISHED,

  // Sent when panel gains/loses focus.
  // The source is the Panel, no details.
  // Used only in unit testing.
  NOTIFICATION_PANEL_CHANGED_ACTIVE_STATUS,

  // Sent when panel is minimized/restored/shows title only etc.
  // The source is the Panel, no details.
  NOTIFICATION_PANEL_CHANGED_EXPANSION_STATE,

  // Sent when panel app icon is loaded.
  // Used only in unit testing.
  NOTIFICATION_PANEL_APP_ICON_LOADED,

  // Sent when panel collection get updated.
  // The source is the PanelCollection, no details.
  // Used only in coordination with notification balloons.
  NOTIFICATION_PANEL_COLLECTION_UPDATED,

  // Sent when panel is closed.
  // The source is the Panel, no details.
  NOTIFICATION_PANEL_CLOSED,

  // Sent when a global error has changed and the error UI should update it
  // self. The source is a Source<Profile> containing the profile for the
  // error. The detail is a GlobalError object that has changed or NULL if
  // all error UIs should update.
  NOTIFICATION_GLOBAL_ERRORS_CHANGED,

  // The user accepted or dismissed a SSL client authentication request.
  // The source is a Source<net::HttpNetworkSession>.  Details is a
  // (std::pair<net::SSLCertRequestInfo*, net::X509Certificate*>).
  NOTIFICATION_SSL_CLIENT_AUTH_CERT_SELECTED,

  // Note:-
  // Currently only Content and Chrome define and use notifications.
  // Custom notifications not belonging to Content and Chrome should start
  // from here.
  NOTIFICATION_CHROME_END,
};

}  // namespace chrome

#endif  // CHROME_BROWSER_CHROME_NOTIFICATION_TYPES_H_
