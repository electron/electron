// Copyright 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/component_updater/extension_installer.h"

#include <string.h>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/version.h"
#include "components/component_updater/component_updater_paths.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/constants.h"
#include "base/json/json_string_value_serializer.h"
#include "base/base64.h"
#include "crypto/sha2.h"
#include "base/strings/string_number_conversions.h"
#include "components/crx_file/id_util.h"

using component_updater::ComponentUpdateService;

namespace {
  bool RewriteManifestFile(
    const base::FilePath& extension_root,
    const base::DictionaryValue& manifest,
    const std::string &public_key) {

  // Add the public key
  DCHECK(!public_key.empty());

  std::unique_ptr<base::DictionaryValue> final_manifest(manifest.DeepCopy());
  final_manifest->SetString(extensions::manifest_keys::kPublicKey, public_key);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(*final_manifest)) {
    return false;
  }

  base::FilePath manifest_path =
    extension_root.Append(extensions::kManifestFilename);
  int size = base::checked_cast<int>(manifest_json.size());
  if (base::WriteFile(manifest_path, manifest_json.data(), size) != size) {
    return false;
  }
  return true;
}

}  // namespace

namespace component_updater {

ExtensionInstallerTraits::ExtensionInstallerTraits(
    const std::string &public_key) : public_key_(public_key) {
}

bool ExtensionInstallerTraits::CanAutoUpdate() const {
  return true;
}

bool ExtensionInstallerTraits::RequiresNetworkEncryption() const {
  return false;
}

bool ExtensionInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return true;  // Nothing custom here.
}

void ExtensionInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  // LOG(INFO) << "Component ready, version " << version.GetString() << " in "
  //  << install_dir.value();
}

// Called during startup and installation before ComponentReady().
bool ExtensionInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {

  // The manifest file will generate a random ID if we don't provide one.
  // We want to write one with the actual extensions public key so we get
  // the same extensionID which is generated from the public key.
  std::string base64_public_key;
  base::Base64Encode(public_key_, &base64_public_key);
  if (!RewriteManifestFile(install_dir, manifest, base64_public_key)) {
    return false;
  }
  return base::PathExists(
      install_dir.Append(FILE_PATH_LITERAL("manifest.json")));
}

base::FilePath
ExtensionInstallerTraits::GetRelativeInstallDir() const {
  // Get the extension ID from the public key
  std::string extension_id = crx_file::id_util::GenerateId(public_key_);
  return base::FilePath(
      // Convert to wstring or string depending on OS
      base::FilePath::StringType(extension_id.begin(), extension_id.end()));
}

void ExtensionInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  const std::string public_key_sha256 = crypto::SHA256HashString(public_key_);
  hash->assign(public_key_sha256.begin(), public_key_sha256.end());
}

std::string ExtensionInstallerTraits::GetName() const {
  return "";
}

std::string ExtensionInstallerTraits::GetAp() const {
  return std::string();
}

void RegisterExtension(
    ComponentUpdateService* cus,
    const std::string& public_key,
    const base::Closure& callback) {
  std::unique_ptr<ComponentInstallerTraits> traits(
      new ExtensionInstallerTraits(public_key));

  // |cus| will take ownership of |installer| during installer->Register(cus).
  DefaultComponentInstaller* installer =
      new DefaultComponentInstaller(std::move(traits));
  installer->Register(cus, callback);
}

}  // namespace component_updater
