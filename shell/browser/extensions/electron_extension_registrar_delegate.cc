// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_registrar_delegate.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

using LoadErrorBehavior = ExtensionRegistrar::LoadErrorBehavior;

namespace {

ElectronExtensionRegistrarDelegate::ElectronExtensionRegistrarDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}

ElectronExtensionRegistrarDelegate::~ElectronExtensionRegistrarDelegate() =
    default;

void ElectronExtensionRegistrarDelegate::PreAddExtension(
    const Extension* extension,
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

void ElectronExtensionRegistrarDelegate::PostActivateExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionRegistrarDelegate::PostDeactivateExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionRegistrarDelegate::PreUninstallExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionRegistrarDelegate::PostUninstallExtension(
    scoped_refptr<const Extension> extension,
    base::OnceClosure done_callback) {}

void ElectronExtensionRegistrarDelegate::PostNotifyUninstallExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionRegistrarDelegate::LoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path,
    LoadErrorBehavior load_error_behavior) {
  CHECK(!path.empty());

  // TODO(nornagon): we should save whether file access was granted
  // when loading this extension and retain it here. As is, reloading an
  // extension will cause the file access permission to be dropped.
  int load_flags = Extension::FOLLOW_SYMLINKS_ANYWHERE;
  GetExtensionFileTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadUnpacked, path, load_flags),
      base::BindOnce(&ElectronExtensionRegistrarDelegate::FinishExtensionReload,
                     weak_factory_.GetWeakPtr(), extension_id));
  did_schedule_reload_ = true;
}

void ElectronExtensionRegistrarDelegate::ShowExtensionDisabledError(
    const Extension* extension,
    bool is_remote_install) {}

void ElectronExtensionRegistrarDelegate::FinishDelayedInstallationsIfAny() {}

bool ElectronExtensionRegistrarDelegate::CanAddExtension(
    const Extension* extension) {
  return true;
}

bool ElectronExtensionRegistrarDelegate::CanEnableExtension(
    const Extension* extension) {
  return true;
}

bool ElectronExtensionRegistrarDelegate::CanDisableExtension(
    const Extension* extension) {
  // Extensions cannot be disabled by the user.
  return false;
}

bool ElectronExtensionRegistrarDelegate::ShouldBlockExtension(
    const Extension* extension) {
  return false;
}

}  // namespace extensions
