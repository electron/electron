// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_loader.h"

#include <optional>
#include <utility>

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "base/version.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/api/declarative_net_request/install_index_helper.h"
#include "extensions/browser/api/declarative_net_request/prefs_helper.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_pref_names.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/pref_names.h"
#include "extensions/browser/ruleset_parse_result.h"
#include "extensions/browser/service_worker/service_worker_task_queue.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/file_util.h"
#include "extensions/common/install_warning.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "third_party/blink/public/common/service_worker/service_worker_status_code.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"

namespace extensions {

// What loading an unpacked extension from disk produced.
struct UnpackedExtensionLoadResult {
  scoped_refptr<const Extension> extension;

  // Why the load failed if `extension` is null, and its warnings otherwise.
  std::string error_or_warnings;

  // What ExtensionPrefs needs to store about each of the extension's static
  // declarativeNetRequest rulesets, keyed by ruleset ID. Empty if they could
  // not be indexed.
  base::DictValue ruleset_install_prefs;
};

namespace {

// Extension pref holding the version of the extension this session loaded
// last.
constexpr char kPrefLastLoadedVersion[] = "electron_last_loaded_version";

UnpackedExtensionLoadResult LoadUnpacked(const base::FilePath& extension_dir,
                                         int load_flags) {
  // app_shell only supports unpacked extensions.
  // NOTE: If you add packed extension support consider removing the flag
  // FOLLOW_SYMLINKS_ANYWHERE below. Packed extensions should not have symlinks.
  if (!base::DirectoryExists(extension_dir)) {
    std::string err = "Extension directory not found: " +
                      base::UTF16ToUTF8(extension_dir.LossyDisplayName());
    return {.error_or_warnings = std::move(err)};
  }

  std::u16string load_error;
  scoped_refptr<Extension> extension = file_util::LoadExtension(
      extension_dir, extensions::mojom::ManifestLocation::kCommandLine,
      load_flags, &load_error);
  if (!extension.get()) {
    std::string err = "Loading extension at " +
                      base::UTF16ToUTF8(extension_dir.LossyDisplayName()) +
                      " failed with: " + base::UTF16ToUTF8(load_error);
    return {.error_or_warnings = std::move(err)};
  }

  // RulesMonitorService only loads a static declarativeNetRequest ruleset
  // whose checksum is in ExtensionPrefs, and the checksum comes from indexing
  // the ruleset. The indexed rulesets are written to the extension's _metadata
  // folder.
  RulesetParseResult index_result = declarative_net_request::
      InstallIndexHelper::IndexAndPersistRulesOnInstall(*extension);
  base::DictValue ruleset_install_prefs;
  if (index_result.error) {
    // Indexing fails if the extension directory is read-only, which is common
    // for an extension that ships inside an app. Such an extension works
    // without its static rulesets, so warn rather than fail the load. None of
    // the rulesets are loaded, because indexing stops at the first failure.
    extension->AddInstallWarning(InstallWarning(
        "Static declarativeNetRequest rulesets were not loaded: " +
        *index_result.error));
  } else {
    extension->AddInstallWarnings(std::move(index_result.warnings));
    ruleset_install_prefs = std::move(index_result.ruleset_install_prefs);
  }

  std::string warnings;
  // Log warnings.
  if (!extension->install_warnings().empty()) {
    std::string warning_prefix =
        "Warnings loading extension at " +
        base::UTF16ToUTF8(extension_dir.LossyDisplayName());

    // Chromium warns about every top-level name that starts with "_". The
    // _metadata folder is expected: it holds the indexed rulesets, and an
    // extension from the Chrome Web Store ships with one.
    const std::string metadata_folder_warning = base::UTF16ToUTF8(
        file_util::ReservedFilenameWarning(base::FilePath(kMetadataFolder)));

    for (const auto& warning : extension->install_warnings()) {
      std::string unrecognized_manifest_error = ErrorUtils::FormatErrorMessage(
          manifest_errors::kUnrecognizedManifestKey, warning.key);

      if (warning.message == unrecognized_manifest_error) {
        // filter kUnrecognizedManifestKey error. This error does not have any
        // impact e.g: Unrecognized manifest key 'minimum_chrome_version' etc.
        LOG(WARNING) << warning_prefix << ": " << warning.message;
      } else if (warning.message == metadata_folder_warning) {
        continue;
      } else {
        warnings += "  " + warning.message + "\n";
      }
    }

    if (warnings != "") {
      warnings = warning_prefix + ":\n" + warnings;
    }
  }

  return {.extension = std::move(extension),
          .error_or_warnings = std::move(warnings),
          .ruleset_install_prefs = std::move(ruleset_install_prefs)};
}

// Stores what RulesMonitorService needs in order to load `extension`'s static
// declarativeNetRequest rulesets. Must be called before the extension is added.
//
// ExtensionPrefs::OnExtensionInstalled() would also store this, but it replaces
// everything stored about the extension's static rulesets. An extension is
// loaded again on every launch, so that would drop the rulesets and rules the
// extension enabled or disabled with the declarativeNetRequest API. Only
// replace the entry of each ruleset instead.
void StoreRulesetInstallPrefs(ExtensionPrefs* extension_prefs,
                              const Extension& extension,
                              base::DictValue ruleset_install_prefs) {
  for (const auto& ruleset :
       declarative_net_request::DNRManifestData::GetRulesets(extension)) {
    const std::string ruleset_id = base::NumberToString(ruleset.id.value());
    // A ruleset that was not indexed has no entry, which removes a checksum
    // stored by an earlier load.
    std::optional<base::Value> ruleset_prefs =
        ruleset_install_prefs.Extract(ruleset_id);
    extension_prefs->UpdateExtensionPref(
        extension.id(),
        ExtensionPrefs::JoinPrefs(
            {ExtensionPrefs::kDNRStaticRulesetPref, ruleset_id}),
        std::move(ruleset_prefs));
  }
}

}  // namespace

ElectronExtensionLoader::ElectronExtensionLoader(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      extension_registrar_(ExtensionRegistrar::Get(browser_context)) {
  extension_registrar_->Init(
      this, /*extensions_enabled=*/true, base::CommandLine::ForCurrentProcess(),
      browser_context_->GetPath().AppendASCII(kInstallDirectoryName),
      browser_context_->GetPath().AppendASCII(kUnpackedInstallDirectoryName));
}

ElectronExtensionLoader::~ElectronExtensionLoader() = default;

void ElectronExtensionLoader::LoadExtension(
    const base::FilePath& extension_dir,
    int load_flags,
    base::OnceCallback<void(const Extension*, const std::string&)> cb) {
  GetExtensionFileTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadUnpacked, extension_dir, load_flags),
      base::BindOnce(&ElectronExtensionLoader::FinishExtensionLoad,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
}

void ElectronExtensionLoader::ReloadExtension(const ExtensionId& extension_id) {
  const Extension* extension = ExtensionRegistry::Get(browser_context_)
                                   ->GetInstalledExtension(extension_id);
  // We shouldn't be trying to reload extensions that haven't been added.
  DCHECK(extension);

  // This should always start false since it's only set here, or in
  // LoadExtensionForReload() as a result of the call below.
  DCHECK_EQ(false, did_schedule_reload_);
  base::AutoReset<bool> reset_did_schedule_reload(&did_schedule_reload_, false);

  extension_registrar_->ReloadExtensionWithQuietFailure(extension_id);
  if (did_schedule_reload_)
    return;
}

void ElectronExtensionLoader::UnloadExtension(
    const ExtensionId& extension_id,
    extensions::UnloadedExtensionReason reason) {
  extension_registrar_->RemoveExtension(extension_id, reason);
}

void ElectronExtensionLoader::FinishExtensionLoad(
    base::OnceCallback<void(const Extension*, const std::string&)> cb,
    UnpackedExtensionLoadResult result) {
  scoped_refptr<const Extension> extension = result.extension;
  base::OnceClosure add_extension = base::BindOnce(
      &ElectronExtensionLoader::AddLoadedExtension, weak_factory_.GetWeakPtr(),
      std::move(cb), std::move(result));

  // A worker registered by a previous launch keeps running the script it was
  // registered with, and the extension's files can change between launches
  // without its version changing. Unregister it so the worker is registered
  // again from the files being loaded now.
  auto* task_queue = ServiceWorkerTaskQueue::Get(browser_context_);
  if (extension && BackgroundInfo::IsServiceWorkerBased(extension.get()) &&
      task_queue->RetrieveRegisteredServiceWorkerVersion(extension->id())
          .IsValid()) {
    task_queue->RemoveRegisteredServiceWorkerInfo(extension->id());
    util::GetStoragePartitionForExtensionId(extension->id(), browser_context_)
        ->GetServiceWorkerContext()
        ->UnregisterServiceWorkerImmediately(
            extension->url(),
            blink::StorageKey::CreateFirstParty(extension->origin()),
            base::IgnoreArgs<blink::ServiceWorkerStatusCode>(
                std::move(add_extension)));
    return;
  }

  std::move(add_extension).Run();
}

void ElectronExtensionLoader::AddLoadedExtension(
    base::OnceCallback<void(const Extension*, const std::string&)> cb,
    UnpackedExtensionLoadResult result) {
  scoped_refptr<const Extension> extension = std::move(result.extension);
  if (extension) {
    ExtensionPrefs* extension_prefs = ExtensionPrefs::Get(browser_context_);

    if (BackgroundInfo::IsServiceWorkerBased(extension.get())) {
      // Tell Chromium that it needs to start the extension's service worker.
      // Chromium usually does this only when an extension is first installed
      // because Chrome will restart the service worker when the browser
      // relaunches. In Electron, we make a fresh install on every app start,
      // so we need to run the fresh install logic again.
      extension_prefs->UpdateExtensionPref(
          extension.get()->id(), extensions::kPrefHasStartedServiceWorker,
          base::Value(false));
    }

    // The version this session loaded last tells a first install and an update
    // apart from a relaunch.
    std::string last_loaded_version;
    extension_prefs->ReadPrefAsString(extension->id(), kPrefLastLoadedVersion,
                                      &last_loaded_version);
    const base::Version previous_version(last_loaded_version);

    // GlobalRulesTracker starts every launch with no declarativeNetRequest
    // rules allocated to this extension, but the allocation it stores in
    // ExtensionPrefs outlives the launch. Drop the stored one so that the two
    // agree. Rules are allocated again when the rulesets are loaded.
    if (loaded_extension_ids_.insert(extension->id()).second) {
      declarative_net_request::PrefsHelper(*extension_prefs)
          .SetAllocatedGlobalRuleCount(extension->id(), /*rule_count=*/0);
    }

    StoreRulesetInstallPrefs(extension_prefs, *extension,
                             std::move(result.ruleset_install_prefs));

    extension_registrar_->AddExtension(extension);

    // Chromium dispatches runtime.onInstalled from its install flow, which
    // loading an unpacked extension on every launch does not go through.
    if (!previous_version.IsValid() ||
        previous_version != extension->version()) {
      extension_prefs->UpdateExtensionPref(
          extension->id(), kPrefLastLoadedVersion,
          base::Value(extension->VersionString()));
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE,
          base::BindOnce(
              &RuntimeEventRouter::DispatchOnInstalledEvent,
              base::UnsafeDangling(static_cast<void*>(browser_context_.get())),
              extension->id(), previous_version, false));
    }

    // Write extension install time to ExtensionPrefs.
    // This is required by extensions::WebRequestAPI
    // which calls extensions::ExtensionPrefs::GetInstallTime.
    //
    // Implementation for writing the pref was based on
    // PreferenceAPIBase::SetExtensionControlledPref.
    {
      ExtensionPrefs::ScopedDictionaryUpdate update(
          extension_prefs, extension.get()->id(),
          extensions::pref_names::kPrefPreferences);

      auto preference = update.Create();
      const int64_t now_usec =
          base::Time::Now().since_origin().InMicroseconds();
      preference->SetString("install_time", base::NumberToString(now_usec));
    }
  }

  std::move(cb).Run(extension.get(), result.error_or_warnings);
}

void ElectronExtensionLoader::FinishExtensionReload(
    const ExtensionId& old_extension_id,
    UnpackedExtensionLoadResult result) {
  scoped_refptr<const Extension> extension = std::move(result.extension);
  if (extension) {
    StoreRulesetInstallPrefs(ExtensionPrefs::Get(browser_context_), *extension,
                             std::move(result.ruleset_install_prefs));
    extension_registrar_->AddExtension(extension);
  }
}

void ElectronExtensionLoader::PreAddExtension(const Extension* extension,
                                              const Extension* old_extension) {
  if (old_extension)
    return;

  // The extension might be disabled if a previous reload attempt failed. In
  // that case, we want to remove that disable reason.
  ExtensionPrefs* extension_prefs = ExtensionPrefs::Get(browser_context_);
  extension_prefs->RemoveDisableReason(extension->id(),
                                       disable_reason::DISABLE_RELOAD);
}

void ElectronExtensionLoader::PostActivateExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionLoader::PostDeactivateExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionLoader::PreUninstallExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionLoader::PostUninstallExtension(
    scoped_refptr<const Extension> extension,
    base::OnceClosure done_callback) {}

void ElectronExtensionLoader::DoLoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path) {
  CHECK(!path.empty());

  // TODO(nornagon): we should save whether file access was granted
  // when loading this extension and retain it here. As is, reloading an
  // extension will cause the file access permission to be dropped.
  int load_flags = Extension::FOLLOW_SYMLINKS_ANYWHERE;
  GetExtensionFileTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadUnpacked, path, load_flags),
      base::BindOnce(&ElectronExtensionLoader::FinishExtensionReload,
                     weak_factory_.GetWeakPtr(), extension_id));
  did_schedule_reload_ = true;
}

void ElectronExtensionLoader::LoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path) {
  DoLoadExtensionForReload(extension_id, path);
}

void ElectronExtensionLoader::LoadExtensionForReloadWithQuietFailure(
    const ExtensionId& extension_id,
    const base::FilePath& path) {
  DoLoadExtensionForReload(extension_id, path);
}

void ElectronExtensionLoader::ShowExtensionDisabledError(
    const Extension* extension,
    bool is_remote_install) {}

bool ElectronExtensionLoader::CanEnableExtension(const Extension* extension) {
  return true;
}

bool ElectronExtensionLoader::CanDisableExtension(const Extension* extension) {
  // Extensions cannot be disabled by the user.
  return false;
}

void ElectronExtensionLoader::GrantActivePermissions(
    const Extension* extension) {
  NOTIMPLEMENTED();
}

}  // namespace extensions
