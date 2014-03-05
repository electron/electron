// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_BROWSER_H_
#define ATOM_BROWSER_BROWSER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "browser/browser_observer.h"
#include "browser/window_list_observer.h"

namespace atom {

// This class is used for control application-wide operations.
class Browser : public WindowListObserver {
 public:
  Browser();
  ~Browser();

  static Browser* Get();

  // Try to close all windows and quit the application.
  void Quit();

  // Quit the application immediately without cleanup work.
  void Terminate();

  // Focus the application.
  void Focus();

  // Returns the version of the executable (or bundle).
  std::string GetVersion() const;

  // Overrides the application version.
  void SetVersion(const std::string& version);

  // Returns the application's name, default is just Atom-Shell.
  std::string GetName() const;

  // Overrides the application name.
  void SetName(const std::string& name);

#if defined(OS_MACOSX)
  // Bounce the dock icon.
  enum BounceType {
    BOUNCE_CRITICAL = 0,
    BOUNCE_INFORMATIONAL = 10,
  };
  int DockBounce(BounceType type);
  void DockCancelBounce(int request_id);

  // Set/Get dock's badge text.
  void DockSetBadgeText(const std::string& label);
  std::string DockGetBadgeText();
#endif  // defined(OS_MACOSX)

  // Tell the application to open a file.
  bool OpenFile(const std::string& file_path);

  // Tell the application to open a url.
  void OpenURL(const std::string& url);

  // Tell the application that application is activated with no open windows.
  void ActivateWithNoOpenWindows();

  // Tell the application the loading has been done.
  void WillFinishLaunching();
  void DidFinishLaunching();

  void AddObserver(BrowserObserver* obs) {
    observers_.AddObserver(obs);
  }

  void RemoveObserver(BrowserObserver* obs) {
    observers_.RemoveObserver(obs);
  }

  bool is_quiting() const { return is_quiting_; }

 protected:
  // Returns the version of application bundle or executable file.
  std::string GetExecutableFileVersion() const;

  // Returns the name of application bundle or executable file.
  std::string GetExecutableFileProductName() const;

  // Send the will-quit message and then terminate the application.
  void NotifyAndTerminate();

  // Tell the system we have cancelled quiting.
  void CancelQuit();

  bool is_quiting_;

 private:
  // WindowListObserver implementations:
  virtual void OnWindowCloseCancelled(NativeWindow* window) OVERRIDE;
  virtual void OnWindowAllClosed() OVERRIDE;

  // Observers of the browser.
  ObserverList<BrowserObserver> observers_;

  std::string version_override_;
  std::string name_override_;

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

}  // namespace atom

#endif  // ATOM_BROWSER_BROWSER_H_
