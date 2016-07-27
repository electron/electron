// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class gathers state related to a single user profile.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_H_
#define CHROME_BROWSER_PROFILES_PROFILE_H_

#include <string>

#include "base/containers/hash_tables.h"
#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
// #include "components/domain_reliability/clear_mode.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"

class ChromeAppCacheService;
class ChromeZoomLevelPrefs;
class DevToolsNetworkControllerHandle;
class ExtensionSpecialStoragePolicy;
class HostContentSettingsMap;
class PrefProxyConfigTracker;
class PrefService;
class PromoCounter;
class ProtocolHandlerRegistry;
class TestingProfile;

namespace android {
class TabContentsProvider;
}

namespace base {
class SequencedTaskRunner;
class Time;
}

namespace chrome_browser_net {
class Predictor;
}

namespace chromeos {
class LibCrosServiceLibraryImpl;
class ResetDefaultProxyConfigServiceTask;
}

namespace content {
class WebUI;
}

namespace storage {
class FileSystemContext;
}

namespace net {
class SSLConfigService;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// Instead of adding more members to Profile, consider creating a
// KeyedService. See
// http://dev.chromium.org/developers/design-documents/profile-architecture
class Profile : public content::BrowserContext {
 public:
  enum CreateStatus {
    // Profile services were not created due to a local error (e.g., disk full).
    CREATE_STATUS_LOCAL_FAIL,
    // Profile services were not created due to a remote error (e.g., network
    // down during limited-user registration).
    CREATE_STATUS_REMOTE_FAIL,
    // Profile created but before initializing extensions and promo resources.
    CREATE_STATUS_CREATED,
    // Profile is created, extensions and promo resources are initialized.
    CREATE_STATUS_INITIALIZED,
    // Profile creation (supervised-user registration, generally) was canceled
    // by the user.
    CREATE_STATUS_CANCELED,
    MAX_CREATE_STATUS  // For histogram display.
  };

  enum CreateMode {
    CREATE_MODE_SYNCHRONOUS,
    CREATE_MODE_ASYNCHRONOUS
  };

  enum ExitType {
    // A normal shutdown. The user clicked exit/closed last window of the
    // profile.
    EXIT_NORMAL,

    // The exit was the result of the system shutting down.
    EXIT_SESSION_ENDED,

    EXIT_CRASHED,
  };

  enum ProfileType {
    REGULAR_PROFILE,  // Login user's normal profile
    INCOGNITO_PROFILE,  // Login user's off-the-record profile
    GUEST_PROFILE,  // Guest session's profile
  };

  class Delegate {
   public:
    virtual ~Delegate();

    // Called when creation of the profile is finished.
    virtual void OnProfileCreated(Profile* profile,
                                  bool success,
                                  bool is_new_profile) = 0;
  };

  // Key used to bind profile to the widget with which it is associated.
  static const char kProfileKey[];
  // Value representing no hosted domain in the kProfileHostedDomain preference.
  static const char kNoHostedDomainFound[];

  Profile();
  ~Profile() override;

  // Profile prefs are registered as soon as the prefs are loaded for the first
  // time.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Create a new profile given a path. If |create_mode| is
  // CREATE_MODE_ASYNCHRONOUS then the profile is initialized asynchronously.
  static Profile* CreateProfile(const base::FilePath& path,
                                Delegate* delegate,
                                CreateMode create_mode);

  // Returns the profile corresponding to the given browser context.
  static Profile* FromBrowserContext(content::BrowserContext* browser_context);

  // Returns the profile corresponding to the given WebUI.
  static Profile* FromWebUI(content::WebUI* web_ui);

  // content::BrowserContext implementation ------------------------------------

  // Typesafe upcast.
  virtual TestingProfile* AsTestingProfile();

  // Returns sequenced task runner where browser context dependent I/O
  // operations should be performed.
  virtual scoped_refptr<base::SequencedTaskRunner> GetIOTaskRunner() = 0;

  // Returns the username associated with this profile, if any. In non-test
  // implementations, this is usually the Google-services email address.
  virtual std::string GetProfileUserName() const = 0;

  // Returns the profile type.
  virtual ProfileType GetProfileType() const = 0;

  // Return the incognito version of this profile. The returned pointer
  // is owned by the receiving profile. If the receiving profile is off the
  // record, the same profile is returned.
  //
  // WARNING: This will create the OffTheRecord profile if it doesn't already
  // exist. If this isn't what you want, you need to check
  // HasOffTheRecordProfile() first.
  virtual Profile* GetOffTheRecordProfile() = 0;

  // Destroys the incognito profile.
  virtual void DestroyOffTheRecordProfile() = 0;

  // True if an incognito profile exists.
  virtual bool HasOffTheRecordProfile() = 0;

  // Return the original "recording" profile. This method returns this if the
  // profile is not incognito.
  virtual Profile* GetOriginalProfile() = 0;

  // Returns whether the profile is supervised (either a legacy supervised
  // user or a child account; see SupervisedUserService).
  virtual bool IsSupervised() const = 0;
  // Returns whether the profile is associated with a child account.
  virtual bool IsChild() const = 0;
  // Returns whether the profile is a legacy supervised user profile.
  virtual bool IsLegacySupervised() const = 0;

  // Accessor. The instance is created upon first access.
  virtual ExtensionSpecialStoragePolicy*
      GetExtensionSpecialStoragePolicy() = 0;

  // Retrieves a pointer to the PrefService that manages the
  // preferences for this user profile.
  virtual PrefService* GetPrefs() = 0;
  virtual const PrefService* GetPrefs() const = 0;

  // Retrieves a pointer to the PrefService that manages the default zoom
  // level and the per-host zoom levels for this user profile.
  // TODO(wjmaclean): Remove this when HostZoomMap migrates to StoragePartition.
  virtual ChromeZoomLevelPrefs* GetZoomLevelPrefs();

  // Retrieves a pointer to the PrefService that manages the preferences
  // for OffTheRecord Profiles.  This PrefService is lazily created the first
  // time that this method is called.
  virtual PrefService* GetOffTheRecordPrefs() = 0;

  // Returns the main request context.
  virtual net::URLRequestContextGetter* GetRequestContext() = 0;

  // Returns the request context used for extension-related requests.  This
  // is only used for a separate cookie store currently.
  virtual net::URLRequestContextGetter* GetRequestContextForExtensions() = 0;

  // Returns the SSLConfigService for this profile.
  virtual net::SSLConfigService* GetSSLConfigService() = 0;

  // Return whether 2 profiles are the same. 2 profiles are the same if they
  // represent the same profile. This can happen if there is pointer equality
  // or if one profile is the incognito version of another profile (or vice
  // versa).
  virtual bool IsSameProfile(Profile* profile) = 0;

  // Returns the time the profile was started. This is not the time the profile
  // was created, rather it is the time the user started chrome and logged into
  // this profile. For the single profile case, this corresponds to the time
  // the user started chrome.
  virtual base::Time GetStartTime() const = 0;

  // Returns the last directory that was chosen for uploading or opening a file.
  virtual base::FilePath last_selected_directory() = 0;
  virtual void set_last_selected_directory(const base::FilePath& path) = 0;

#if defined(OS_CHROMEOS)
  enum AppLocaleChangedVia {
    // Caused by chrome://settings change.
    APP_LOCALE_CHANGED_VIA_SETTINGS,
    // Locale has been reverted via LocaleChangeGuard.
    APP_LOCALE_CHANGED_VIA_REVERT,
    // From login screen.
    APP_LOCALE_CHANGED_VIA_LOGIN,
    // From login to a public session.
    APP_LOCALE_CHANGED_VIA_PUBLIC_SESSION_LOGIN,
    // Source unknown.
    APP_LOCALE_CHANGED_VIA_UNKNOWN
  };

  // Changes application locale for a profile.
  virtual void ChangeAppLocale(
      const std::string& locale, AppLocaleChangedVia via) = 0;

  // Called after login.
  virtual void OnLogin() = 0;

  // Initializes Chrome OS's preferences.
  virtual void InitChromeOSPreferences() = 0;
#endif  // defined(OS_CHROMEOS)

  // Returns the helper object that provides the proxy configuration service
  // access to the the proxy configuration possibly defined by preferences.
  virtual PrefProxyConfigTracker* GetProxyConfigTracker() = 0;

  // Returns the Predictor object used for dns prefetch.
  virtual chrome_browser_net::Predictor* GetNetworkPredictor() = 0;

  // Returns the DevToolsNetworkControllerHandle for this profile.
  virtual DevToolsNetworkControllerHandle*
  GetDevToolsNetworkControllerHandle() = 0;

  // Deletes all network related data since |time|. It deletes transport
  // security state since |time| and it also deletes HttpServerProperties data.
  // Works asynchronously, however if the |completion| callback is non-null, it
  // will be posted on the UI thread once the removal process completes.
  // Be aware that theoretically it is possible that |completion| will be
  // invoked after the Profile instance has been destroyed.
  virtual void ClearNetworkingHistorySince(base::Time time,
                                           const base::Closure& completion) = 0;

  // Returns the home page for this profile.
  virtual GURL GetHomePage() = 0;

  // Returns whether or not the profile was created by a version of Chrome
  // more recent (or equal to) the one specified.
  virtual bool WasCreatedByVersionOrLater(const std::string& version) = 0;

  std::string GetDebugName();

  // Returns whether it is a guest session.
  virtual bool IsGuestSession() const;

  // Returns whether it is a system profile.
  virtual bool IsSystemProfile() const;

  // Did the user restore the last session? This is set by SessionRestore.
  void set_restored_last_session(bool restored_last_session) {
    restored_last_session_ = restored_last_session;
  }
  bool restored_last_session() const {
    return restored_last_session_;
  }

  // Sets the ExitType for the profile. This may be invoked multiple times
  // during shutdown; only the first such change (the transition from
  // EXIT_CRASHED to one of the other values) is written to prefs, any
  // later calls are ignored.
  //
  // NOTE: this is invoked internally on a normal shutdown, but is public so
  // that it can be invoked when the user logs out/powers down (WM_ENDSESSION),
  // or to handle backgrounding/foregrounding on mobile.
  virtual void SetExitType(ExitType exit_type) = 0;

  // Returns how the last session was shutdown.
  virtual ExitType GetLastSessionExitType() = 0;

  // Stop sending accessibility events until ResumeAccessibilityEvents().
  // Calls to Pause nest; no events will be sent until the number of
  // Resume calls matches the number of Pause calls received.
  void PauseAccessibilityEvents() {
    accessibility_pause_level_++;
  }

  void ResumeAccessibilityEvents() {
    DCHECK_GT(accessibility_pause_level_, 0);
    accessibility_pause_level_--;
  }

  bool ShouldSendAccessibilityEvents() {
    return 0 == accessibility_pause_level_;
  }

  // Returns whether the profile is new.  A profile is new if the browser has
  // not been shut down since the profile was created.
  bool IsNewProfile();

  // Checks whether sync is configurable by the user. Returns false if sync is
  // disallowed by the command line or controlled by configuration management.
  bool IsSyncAllowed();

  // Send NOTIFICATION_PROFILE_DESTROYED for this Profile, if it has not
  // already been sent. It is necessary because most Profiles are destroyed by
  // ProfileDestroyer, but in tests, some are not.
  void MaybeSendDestroyedNotification();

  // Creates an OffTheRecordProfile which points to this Profile.
  Profile* CreateOffTheRecordProfile();

  // Convenience method to retrieve the default zoom level for the default
  // storage partition.
  double GetDefaultZoomLevelForProfile();

 protected:
  void set_is_guest_profile(bool is_guest_profile) {
    is_guest_profile_ = is_guest_profile;
  }

  void set_is_system_profile(bool is_system_profile) {
    is_system_profile_ = is_system_profile;
  }

 private:
  bool restored_last_session_;

  // Used to prevent the notification that this Profile is destroyed from
  // being sent twice.
  bool sent_destroyed_notification_;

  // Accessibility events will only be propagated when the pause
  // level is zero.  PauseAccessibilityEvents and ResumeAccessibilityEvents
  // increment and decrement the level, respectively, rather than set it to
  // true or false, so that calls can be nested.
  int accessibility_pause_level_;

  bool is_guest_profile_;

  // A non-browsing profile not associated to a user. Sample use: User-Manager.
  bool is_system_profile_;

  DISALLOW_COPY_AND_ASSIGN(Profile);
};

// The comparator for profile pointers as key in a map.
struct ProfileCompare {
  bool operator()(Profile* a, Profile* b) const;
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_H_
