// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_process_manager_delegate.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "brave/browser/brave_browser_context.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/common/one_shot_event.h"

using brave::BraveBrowserContext;

namespace extensions {

AtomProcessManagerDelegate::AtomProcessManagerDelegate() {
  registrar_.Add(this,
                 chrome::NOTIFICATION_PROFILE_CREATED,
                 content::NotificationService::AllSources());
  registrar_.Add(this,
                 chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::NotificationService::AllSources());
}

AtomProcessManagerDelegate::~AtomProcessManagerDelegate() {
}

bool AtomProcessManagerDelegate::IsBackgroundPageAllowed(
    content::BrowserContext* context) const {
  return true;
}

bool AtomProcessManagerDelegate::DeferCreatingStartupBackgroundHosts(
    content::BrowserContext* context) const {
  return !atom::Browser::Get()->is_ready();
}

void AtomProcessManagerDelegate::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_PROFILE_CREATED: {
      BraveBrowserContext* browser_context =
        content::Source<BraveBrowserContext>(source).ptr();
      OnProfileCreated(browser_context);
      break;
    }
    case chrome::NOTIFICATION_PROFILE_DESTROYED: {
      BraveBrowserContext* browser_context =
        content::Source<BraveBrowserContext>(source).ptr();
      OnProfileDestroyed(browser_context);
      break;
    }
    default:
      NOTREACHED();
  }
}

void AtomProcessManagerDelegate::OnProfileCreated(
                                        BraveBrowserContext* browser_context) {
  // Incognito browser_contexts are handled by their original browser_context.
  if (browser_context->IsOffTheRecord() || browser_context->HasParentContext())
    return;

  // The browser_context can be created before the extension system is ready.
  if (!ExtensionSystem::Get(browser_context)->ready().is_signaled())
    return;

  // The browser_context might have been initialized asynchronously (in
  // parallel with extension system startup). Now that initialization is
  // complete the ProcessManager can load deferred background pages.
  ProcessManager::Get(browser_context)->MaybeCreateStartupBackgroundHosts();
}

void AtomProcessManagerDelegate::OnProfileDestroyed(
                                        BraveBrowserContext* browser_context) {
  ProcessManager* manager =
      ProcessManagerFactory::GetForBrowserContextIfExists(browser_context);
  if (manager) {
    manager->CloseBackgroundHosts();
  }
}

}  // namespace extensions
