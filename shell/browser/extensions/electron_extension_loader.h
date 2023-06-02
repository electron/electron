// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
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

// Handles extension loading and reloading using ExtensionRegistrar.
class ElectronExtensionLoader : public ExtensionRegistrar::Delegate {
 public:
  explicit ElectronExtensionLoader(content::BrowserContext* browser_context);
  ~ElectronExtensionLoader() override;

  // disable copy
  ElectronExtensionLoader(const ElectronExtensionLoader&) = delete;
  ElectronExtensionLoader& operator=(const ElectronExtensionLoader&) = delete;

  // Loads an unpacked extension from a directory synchronously. Returns the
  // extension on success, or nullptr otherwise.
  void LoadExtension(const base::FilePath& extension_dir,
                     int load_flags,
                     base::OnceCallback<void(const Extension* extension,
                                             const std::string&)> cb);

  // Starts reloading the extension. A keep-alive is maintained until the
  // reload succeeds/fails. If the extension is an app, it will be launched upon
  // reloading.
  // This may invalidate references to the old Extension object, so it takes the
  // ID by value.
  void ReloadExtension(const ExtensionId& extension_id);

  void UnloadExtension(const ExtensionId& extension_id,
                       extensions::UnloadedExtensionReason reason);

  ExtensionRegistrar* registrar() { return &extension_registrar_; }

 private:
  // If the extension loaded successfully, enables it. If it's an app, launches
  // it. If the load failed, updates ShellKeepAliveRequester.
  void FinishExtensionReload(
      const ExtensionId& old_extension_id,
      std::pair<scoped_refptr<const Extension>, std::string> result);

  void FinishExtensionLoad(
      base::OnceCallback<void(const Extension*, const std::string&)> cb,
      std::pair<scoped_refptr<const Extension>, std::string> result);

  // ExtensionRegistrar::Delegate:
  void PreAddExtension(const Extension* extension,
                       const Extension* old_extension) override;
  void PostActivateExtension(scoped_refptr<const Extension> extension) override;
  void PostDeactivateExtension(
      scoped_refptr<const Extension> extension) override;
  void LoadExtensionForReload(
      const ExtensionId& extension_id,
      const base::FilePath& path,
      ExtensionRegistrar::LoadErrorBehavior load_error_behavior) override;
  bool CanEnableExtension(const Extension* extension) override;
  bool CanDisableExtension(const Extension* extension) override;
  bool ShouldBlockExtension(const Extension* extension) override;

  raw_ptr<content::BrowserContext> browser_context_;  // Not owned.

  // Registers and unregisters extensions.
  ExtensionRegistrar extension_registrar_;

  // Holds keep-alives for relaunching apps.
  //   ShellKeepAliveRequester keep_alive_requester_;

  // Indicates that we posted the (asynchronous) task to start reloading.
  // Used by ReloadExtension() to check whether ExtensionRegistrar calls
  // LoadExtensionForReload().
  bool did_schedule_reload_ = false;

  base::WeakPtrFactory<ElectronExtensionLoader> weak_factory_{this};
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_
