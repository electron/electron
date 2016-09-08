// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_PROCESS_MANAGER_DELEGATE_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_PROCESS_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/process_manager_delegate.h"

namespace atom {
class Browser;
}

namespace brave {
class BraveBrowserContext;
}

namespace extensions {

// Support for ProcessManager. Controls cases where Chrome wishes to disallow
// extension background pages or defer their creation.
class AtomProcessManagerDelegate : public ProcessManagerDelegate,
                                   public content::NotificationObserver {
 public:
  AtomProcessManagerDelegate();
  ~AtomProcessManagerDelegate() override;

  // ProcessManagerDelegate implementation:
  bool IsBackgroundPageAllowed(content::BrowserContext* context) const override;
  bool DeferCreatingStartupBackgroundHosts(
      content::BrowserContext* context) const override;

  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  // Notification handlers.
  void OnBrowserWindowReady(atom::Browser* browser);
  void OnProfileCreated(brave::BraveBrowserContext* profile);
  void OnProfileDestroyed(brave::BraveBrowserContext* profile);

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(AtomProcessManagerDelegate);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_PROCESS_MANAGER_DELEGATE_H_
