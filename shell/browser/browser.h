// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_BROWSER_H_
#define SHELL_BROWSER_BROWSER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/values.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/window_list_observer.h"
#include "shell/common/gin_helper/promise.h"

#if defined(OS_WIN)
#include <windows.h>
#include "base/files/file_path.h"
#endif

#if defined(OS_MACOSX)
#include "ui/base/cocoa/secure_password_input.h"
#endif

namespace base {
class FilePath;
}

namespace gfx {
class Image;
}

namespace gin_helper {
class Arguments;
}

namespace electron {

class ElectronMenuModel;

// This class is used for control application-wide operations.
class Browser : public WindowListObserver {
 public:
  Browser();
  ~Browser() override;

  static Browser* Get();

  // Try to close all windows and quit the application.
  void Quit();

  // Exit the application immediately and set exit code.
  void Exit(gin_helper::Arguments* args);

  // Cleanup everything and shutdown the application gracefully.
  void Shutdown();

  // Focus the application.
  void Focus(gin_helper::Arguments* args);

  // Returns the version of the executable (or bundle).
  std::string GetVersion() const;

  // Overrides the application version.
  void SetVersion(const std::string& version);

  // Returns the application's name, default is just Electron.
  std::string GetName() const;

  // Overrides the application name.
  void SetName(const std::string& name);

  // Add the |path| to recent documents list.
  void AddRecentDocument(const base::FilePath& path);

  // Clear the recent documents list.
  void ClearRecentDocuments();

  // Set the application user model ID.
  void SetAppUserModelID(const base::string16& name);

  // Remove the default protocol handler registry key
  bool RemoveAsDefaultProtocolClient(const std::string& protocol,
                                     gin_helper::Arguments* args);

  // Set as default handler for a protocol.
  bool SetAsDefaultProtocolClient(const std::string& protocol,
                                  gin_helper::Arguments* args);

  // Query the current state of default handler for a protocol.
  bool IsDefaultProtocolClient(const std::string& protocol,
                               gin_helper::Arguments* args);

  base::string16 GetApplicationNameForProtocol(const GURL& url);

  // Set/Get the badge count.
  bool SetBadgeCount(int count);
  int GetBadgeCount();

  // Set/Get the login item settings of the app
  struct LoginItemSettings {
    bool open_at_login = false;
    bool open_as_hidden = false;
    bool restore_state = false;
    bool opened_at_login = false;
    bool opened_as_hidden = false;
    base::string16 path;
    std::vector<base::string16> args;

    LoginItemSettings();
    ~LoginItemSettings();
    LoginItemSettings(const LoginItemSettings&);
  };
  void SetLoginItemSettings(LoginItemSettings settings);
  LoginItemSettings GetLoginItemSettings(const LoginItemSettings& options);

#if defined(OS_MACOSX)
  // Set the handler which decides whether to shutdown.
  void SetShutdownHandler(base::Callback<bool()> handler);

  // Hide the application.
  void Hide();

  // Show the application.
  void Show();

  // Creates an activity and sets it as the one currently in use.
  void SetUserActivity(const std::string& type,
                       base::DictionaryValue user_info,
                       gin_helper::Arguments* args);

  // Returns the type name of the current user activity.
  std::string GetCurrentActivityType();

  // Invalidates an activity and marks it as no longer eligible for
  // continuation
  void InvalidateCurrentActivity();

  // Marks this activity object as inactive without invalidating it.
  void ResignCurrentActivity();

  // Updates the current user activity
  void UpdateCurrentActivity(const std::string& type,
                             base::DictionaryValue user_info);

  // Indicates that an user activity is about to be resumed.
  bool WillContinueUserActivity(const std::string& type);

  // Indicates a failure to resume a Handoff activity.
  void DidFailToContinueUserActivity(const std::string& type,
                                     const std::string& error);

  // Resumes an activity via hand-off.
  bool ContinueUserActivity(const std::string& type,
                            base::DictionaryValue user_info);

  // Indicates that an activity was continued on another device.
  void UserActivityWasContinued(const std::string& type,
                                base::DictionaryValue user_info);

  // Gives an oportunity to update the Handoff payload.
  bool UpdateUserActivityState(const std::string& type,
                               base::DictionaryValue user_info);

  // Bounce the dock icon.
  enum class BounceType{
      CRITICAL = 0,        // NSCriticalRequest
      INFORMATIONAL = 10,  // NSInformationalRequest
  };
  int DockBounce(BounceType type);
  void DockCancelBounce(int request_id);

  // Bounce the Downloads stack.
  void DockDownloadFinished(const std::string& filePath);

  // Set/Get dock's badge text.
  void DockSetBadgeText(const std::string& label);
  std::string DockGetBadgeText();

  // Hide/Show dock.
  void DockHide();
  v8::Local<v8::Promise> DockShow(v8::Isolate* isolate);
  bool DockIsVisible();

  // Set docks' menu.
  void DockSetMenu(ElectronMenuModel* model);

  // Set docks' icon.
  void DockSetIcon(const gfx::Image& image);

#endif  // defined(OS_MACOSX)

  void ShowAboutPanel();
  void SetAboutPanelOptions(base::DictionaryValue options);

#if defined(OS_MACOSX) || defined(OS_WIN)
  void ShowEmojiPanel();
#endif

#if defined(OS_WIN)
  struct UserTask {
    base::FilePath program;
    base::string16 arguments;
    base::string16 title;
    base::string16 description;
    base::FilePath working_dir;
    base::FilePath icon_path;
    int icon_index;

    UserTask();
    UserTask(const UserTask&);
    ~UserTask();
  };

  // Add a custom task to jump list.
  bool SetUserTasks(const std::vector<UserTask>& tasks);

  // Returns the application user model ID, if there isn't one, then create
  // one from app's name.
  // The returned string managed by Browser, and should not be modified.
  PCWSTR GetAppUserModelID();
#endif  // defined(OS_WIN)

#if defined(OS_LINUX)
  // Whether Unity launcher is running.
  bool IsUnityRunning();
#endif  // defined(OS_LINUX)

  // Tell the application to open a file.
  bool OpenFile(const std::string& file_path);

  // Tell the application to open a url.
  void OpenURL(const std::string& url);

#if defined(OS_MACOSX)
  // Tell the application to create a new window for a tab.
  void NewWindowForTab();
#endif  // defined(OS_MACOSX)

  // Tell the application that application is activated with visible/invisible
  // windows.
  void Activate(bool has_visible_windows);

  bool IsEmojiPanelSupported();

  // Tell the application the loading has been done.
  void WillFinishLaunching();
  void DidFinishLaunching(base::DictionaryValue launch_info);

  void OnAccessibilitySupportChanged();

  void PreMainMessageLoopRun();
  void PreCreateThreads();

  // Stores the supplied |quit_closure|, to be run when the last Browser
  // instance is destroyed.
  void SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure);

  void AddObserver(BrowserObserver* obs) { observers_.AddObserver(obs); }

  void RemoveObserver(BrowserObserver* obs) { observers_.RemoveObserver(obs); }

#if defined(OS_MACOSX)
  // Returns whether secure input is enabled
  bool IsSecureKeyboardEntryEnabled();
  void SetSecureKeyboardEntryEnabled(bool enabled);
#endif

  bool is_shutting_down() const { return is_shutdown_; }
  bool is_quiting() const { return is_quiting_; }
  bool is_ready() const { return is_ready_; }
  v8::Local<v8::Value> WhenReady(v8::Isolate* isolate);

 protected:
  // Returns the version of application bundle or executable file.
  std::string GetExecutableFileVersion() const;

  // Returns the name of application bundle or executable file.
  std::string GetExecutableFileProductName() const;

  // Send the will-quit message and then shutdown the application.
  void NotifyAndShutdown();

  // Send the before-quit message and start closing windows.
  bool HandleBeforeQuit();

  bool is_quiting_ = false;

 private:
  // WindowListObserver implementations:
  void OnWindowCloseCancelled(NativeWindow* window) override;
  void OnWindowAllClosed() override;

  // Observers of the browser.
  base::ObserverList<BrowserObserver> observers_;

  // Whether `app.exit()` has been called
  bool is_exiting_ = false;

  // Whether "ready" event has been emitted.
  bool is_ready_ = false;

  // The browser is being shutdown.
  bool is_shutdown_ = false;

  // Null until/unless the default main message loop is running.
  base::OnceClosure quit_main_message_loop_;

  int badge_count_ = 0;

  std::unique_ptr<gin_helper::Promise<void>> ready_promise_;

#if defined(OS_MACOSX)
  std::unique_ptr<ui::ScopedPasswordInputEnabler> password_input_enabler_;
#endif

#if defined(OS_LINUX) || defined(OS_WIN)
  base::Value about_panel_options_;
#elif defined(OS_MACOSX)
  base::DictionaryValue about_panel_options_;
#endif

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

}  // namespace electron

#endif  // SHELL_BROWSER_BROWSER_H_
