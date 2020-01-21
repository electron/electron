// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_extension_loader.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_restrictions.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/file_util.h"

namespace extensions {

using LoadErrorBehavior = ExtensionRegistrar::LoadErrorBehavior;

namespace {

scoped_refptr<const Extension> LoadUnpacked(
    const base::FilePath& extension_dir) {
  // app_shell only supports unpacked extensions.
  // NOTE: If you add packed extension support consider removing the flag
  // FOLLOW_SYMLINKS_ANYWHERE below. Packed extensions should not have symlinks.
  // TODO(nornagon): these LOG()s should surface as JS exceptions
  if (!base::DirectoryExists(extension_dir)) {
    LOG(ERROR) << "Extension directory not found: "
               << extension_dir.AsUTF8Unsafe();
    return nullptr;
  }

  int load_flags = Extension::FOLLOW_SYMLINKS_ANYWHERE;
  std::string load_error;
  scoped_refptr<Extension> extension = file_util::LoadExtension(
      extension_dir, Manifest::COMMAND_LINE, load_flags, &load_error);
  if (!extension.get()) {
    LOG(ERROR) << "Loading extension at " << extension_dir.value()
               << " failed with: " << load_error;
    return nullptr;
  }

  // Log warnings.
  if (extension->install_warnings().size()) {
    LOG(WARNING) << "Warnings loading extension at " << extension_dir.value()
                 << ":";
    for (const auto& warning : extension->install_warnings())
      LOG(WARNING) << warning.message;
  }

  return extension;
}

}  // namespace

AtomExtensionLoader::AtomExtensionLoader(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      extension_registrar_(browser_context, this),
      weak_factory_(this) {}

AtomExtensionLoader::~AtomExtensionLoader() = default;

void AtomExtensionLoader::LoadExtension(
    const base::FilePath& extension_dir,
    base::OnceCallback<void(const Extension*)> loaded) {
  base::PostTaskAndReplyWithResult(
      GetExtensionFileTaskRunner().get(), FROM_HERE,
      base::BindOnce(&LoadUnpacked, extension_dir),
      base::BindOnce(&AtomExtensionLoader::FinishExtensionLoad,
                     weak_factory_.GetWeakPtr(), std::move(loaded)));
}

void AtomExtensionLoader::ReloadExtension(const ExtensionId& extension_id) {
  const Extension* extension = ExtensionRegistry::Get(browser_context_)
                                   ->GetInstalledExtension(extension_id);
  // We shouldn't be trying to reload extensions that haven't been added.
  DCHECK(extension);

  // This should always start false since it's only set here, or in
  // LoadExtensionForReload() as a result of the call below.
  DCHECK_EQ(false, did_schedule_reload_);
  base::AutoReset<bool> reset_did_schedule_reload(&did_schedule_reload_, false);

  extension_registrar_.ReloadExtension(extension_id, LoadErrorBehavior::kQuiet);
  if (did_schedule_reload_)
    return;
}

void AtomExtensionLoader::UnloadExtension(
    const ExtensionId& extension_id,
    extensions::UnloadedExtensionReason reason) {
  extension_registrar_.RemoveExtension(extension_id, reason);
}

void AtomExtensionLoader::FinishExtensionLoad(
    base::OnceCallback<void(const Extension*)> done,
    scoped_refptr<const Extension> extension) {
  if (extension) {
    extension_registrar_.AddExtension(extension);
  }
  std::move(done).Run(extension.get());
}

void AtomExtensionLoader::FinishExtensionReload(
    const ExtensionId& old_extension_id,
    scoped_refptr<const Extension> extension) {
  if (extension) {
    extension_registrar_.AddExtension(extension);
  }
}

void AtomExtensionLoader::PreAddExtension(const Extension* extension,
                                          const Extension* old_extension) {
  if (old_extension)
    return;

  // The extension might be disabled if a previous reload attempt failed. In
  // that case, we want to remove that disable reason.
  ExtensionPrefs* extension_prefs = ExtensionPrefs::Get(browser_context_);
  if (extension_prefs->IsExtensionDisabled(extension->id()) &&
      extension_prefs->HasDisableReason(extension->id(),
                                        disable_reason::DISABLE_RELOAD)) {
    extension_prefs->RemoveDisableReason(extension->id(),
                                         disable_reason::DISABLE_RELOAD);
    // Only re-enable the extension if there are no other disable reasons.
    if (extension_prefs->GetDisableReasons(extension->id()) ==
        disable_reason::DISABLE_NONE) {
      extension_prefs->SetExtensionEnabled(extension->id());
    }
  }
}

void AtomExtensionLoader::PostActivateExtension(
    scoped_refptr<const Extension> extension) {}

void AtomExtensionLoader::PostDeactivateExtension(
    scoped_refptr<const Extension> extension) {}

void AtomExtensionLoader::LoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path,
    LoadErrorBehavior load_error_behavior) {
  CHECK(!path.empty());

  base::PostTaskAndReplyWithResult(
      GetExtensionFileTaskRunner().get(), FROM_HERE,
      base::BindOnce(&LoadUnpacked, path),
      base::BindOnce(&AtomExtensionLoader::FinishExtensionReload,
                     weak_factory_.GetWeakPtr(), extension_id));
  did_schedule_reload_ = true;
}

bool AtomExtensionLoader::CanEnableExtension(const Extension* extension) {
  return true;
}

bool AtomExtensionLoader::CanDisableExtension(const Extension* extension) {
  // Extensions cannot be disabled by the user.
  return false;
}

bool AtomExtensionLoader::ShouldBlockExtension(const Extension* extension) {
  return false;
}

}  // namespace extensions
