// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
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

// Handles extension loading.
class ElectronExtensionLoader {
 public:
  explicit ElectronExtensionLoader(content::BrowserContext* browser_context,
                                   ElectronExtensionSystem* extension_system);
  ~ElectronExtensionLoader();

  // disable copy
  ElectronExtensionLoader(const ElectronExtensionLoader&) = delete;
  ElectronExtensionLoader& operator=(const ElectronExtensionLoader&) = delete;

  // Loads an unpacked extension from a directory synchronously. Returns the
  // extension on success, or nullptr otherwise.
  void LoadExtension(const base::FilePath& extension_dir,
                     int load_flags,
                     base::OnceCallback<void(const Extension* extension,
                                             const std::string&)> cb);

 private:
  // If the extension loaded successfully, enables it. If it's an app, launches
  // it. If the load failed, updates ShellKeepAliveRequester.
  void FinishExtensionReload(
      const ExtensionId& old_extension_id,
      std::pair<scoped_refptr<const Extension>, std::string> result);

  void FinishExtensionLoad(
      base::OnceCallback<void(const Extension*, const std::string&)> cb,
      std::pair<scoped_refptr<const Extension>, std::string> result);

  raw_ptr<content::BrowserContext> browser_context_;   // Not owned.
  raw_ptr<ElectronExtensionSystem> extension_system_;  // Not owned.

  base::WeakPtrFactory<ElectronExtensionLoader> weak_factory_{this};
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_
