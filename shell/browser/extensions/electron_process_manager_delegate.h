// Copyright 2019 Slack Technologies, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_PROCESS_MANAGER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_PROCESS_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/process_manager_delegate.h"

class Browser;
class Profile;

namespace extensions {

// Support for ProcessManager. Controls cases where Electron wishes to disallow
// extension background pages or defer their creation.
class ElectronProcessManagerDelegate : public ProcessManagerDelegate {
 public:
  ElectronProcessManagerDelegate();
  ~ElectronProcessManagerDelegate() override;

  // disable copy
  ElectronProcessManagerDelegate(const ElectronProcessManagerDelegate&) =
      delete;
  ElectronProcessManagerDelegate& operator=(
      const ElectronProcessManagerDelegate&) = delete;

  // ProcessManagerDelegate implementation:
  bool AreBackgroundPagesAllowedForContext(
      content::BrowserContext* context) const override;
  bool IsExtensionBackgroundPageAllowed(
      content::BrowserContext* context,
      const Extension& extension) const override;
  bool DeferCreatingStartupBackgroundHosts(
      content::BrowserContext* context) const override;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_PROCESS_MANAGER_DELEGATE_H_
