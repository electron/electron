// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_SHARED_USER_SCRIPT_MASTER_H_
#define ATOM_BROWSER_EXTENSIONS_SHARED_USER_SCRIPT_MASTER_H_

#include <set>

#include "base/scoped_observer.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/browser/extension_user_script_loader.h"
#include "extensions/common/extension.h"
#include "extensions/common/user_script.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ExtensionRegistry;

// Manages statically-defined user scripts for all extensions. Owns a
// UserScriptLoader to which file loading and shared memory management
// operations are delegated.
class SharedUserScriptMaster : public ExtensionRegistryObserver {
 public:
  explicit SharedUserScriptMaster(content::BrowserContext* browser_context);
  ~SharedUserScriptMaster() override;

  // Provides access to loader state method: scripts_ready().
  bool scripts_ready() const { return loader_.scripts_ready(); }

 private:
  // ExtensionRegistryObserver implementation.
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionInfo::Reason reason) override;

  // Gets an extension's scripts' metadata; i.e., gets a list of UserScript
  // objects that contains script info, but not the contents of the scripts.
  const std::set<UserScript> GetScriptsMetadata(const Extension* extension);

  // Script loader that handles loading contents of scripts into shared memory
  // and notifying renderers of scripts in shared memory.
  ExtensionUserScriptLoader loader_;

  // The browser context for which the scripts managed here are installed.
  content::BrowserContext* browser_context_;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      extension_registry_observer_;

  DISALLOW_COPY_AND_ASSIGN(SharedUserScriptMaster);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_SHARED_USER_SCRIPT_MASTER_H_
