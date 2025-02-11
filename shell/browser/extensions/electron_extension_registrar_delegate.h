// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_REGISTRAR_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_REGISTRAR_DELEGATE_H_

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/common/extension_id.h"

namespace base {
class FilePath;
}

namespace content {
class BrowserContext;
}

namespace extensions {

class Extension;
class ElectronExtensionSystem;

// Handles extension loading and reloading using ExtensionRegistrar.
class ElectronExtensionRegistrarDelegate : public ExtensionRegistrar::Delegate {
 public:
  explicit ElectronExtensionRegistrarDelegate(
      content::BrowserContext* browser_context,
      ElectronExtensionSystem* extension_system);
  ~ElectronExtensionRegistrarDelegate() override;

  // disable copy
  ElectronExtensionRegistrarDelegate(
      const ElectronExtensionRegistrarDelegate&) = delete;
  ElectronExtensionRegistrarDelegate& operator=(
      const ElectronExtensionRegistrarDelegate&) = delete;

  void set_extension_registrar(ExtensionRegistrar* registrar) {
    extension_registrar_ = registrar;
  }

 private:
  // ExtensionRegistrar::Delegate:
  void PreAddExtension(const Extension* extension,
                       const Extension* old_extension) override;
  void PostActivateExtension(scoped_refptr<const Extension> extension) override;
  void PostDeactivateExtension(
      scoped_refptr<const Extension> extension) override;
  void PreUninstallExtension(scoped_refptr<const Extension> extension) override;
  void PostUninstallExtension(scoped_refptr<const Extension> extension,
                              base::OnceClosure done_callback) override;
  void PostNotifyUninstallExtension(
      scoped_refptr<const Extension> extension) override;
  void LoadExtensionForReload(
      const ExtensionId& extension_id,
      const base::FilePath& path,
      ExtensionRegistrar::LoadErrorBehavior load_error_behavior) override;
  void ShowExtensionDisabledError(const Extension* extension,
                                  bool is_remote_install) override;
  void FinishDelayedInstallationsIfAny() override;
  bool CanAddExtension(const Extension* extension) override;
  bool CanEnableExtension(const Extension* extension) override;
  bool CanDisableExtension(const Extension* extension) override;
  bool ShouldBlockExtension(const Extension* extension) override;

  // If the extension loaded successfully, enables it. If it's an app, launches
  // it. If the load failed, updates ShellKeepAliveRequester.
  void FinishExtensionReload(const Extension* extension,
                             const ExtensionId& extension_id);

  raw_ptr<content::BrowserContext> browser_context_;   // Not owned.
  raw_ptr<ElectronExtensionSystem> extension_system_;  // Not owned.
  raw_ptr<ExtensionRegistrar> extension_registrar_ = nullptr;

  base::WeakPtrFactory<ElectronExtensionRegistrarDelegate> weak_factory_{this};
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_REGISTRAR_DELEGATE_H_
