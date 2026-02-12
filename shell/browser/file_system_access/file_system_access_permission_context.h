// Copyright (c) 2024 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_ELECTRON_FILE_SYSTEM_ACCESS_PERMISSION_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_ELECTRON_FILE_SYSTEM_ACCESS_PERMISSION_CONTEXT_H_

#include "shell/browser/file_system_access/file_system_access_permission_context.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "chrome/browser/file_system_access/chrome_file_system_access_permission_context.h"  // nogncheck
#include "components/keyed_service/core/keyed_service.h"

class GURL;

namespace gin {
class Arguments;
}  // namespace gin

namespace base {
class FilePath;
}  // namespace base

namespace storage {
class FileSystemURL;
}  // namespace storage

namespace electron {

class FileSystemAccessPermissionContext
    : public KeyedService,
      public content::FileSystemAccessPermissionContext {
 public:
  enum class GrantType { kRead, kWrite };

  explicit FileSystemAccessPermissionContext(
      content::BrowserContext* browser_context,
      const base::Clock* clock = base::DefaultClock::GetInstance());
  FileSystemAccessPermissionContext(const FileSystemAccessPermissionContext&) =
      delete;
  FileSystemAccessPermissionContext& operator=(
      const FileSystemAccessPermissionContext&) = delete;
  ~FileSystemAccessPermissionContext() override;

  // content::FileSystemAccessPermissionContext:
  scoped_refptr<content::FileSystemAccessPermissionGrant>
  GetReadPermissionGrant(const url::Origin& origin,
                         const content::PathInfo& path,
                         HandleType handle_type,
                         UserAction user_action) override;

  scoped_refptr<content::FileSystemAccessPermissionGrant>
  GetWritePermissionGrant(const url::Origin& origin,
                          const content::PathInfo& path,
                          HandleType handle_type,
                          UserAction user_action) override;

  void ConfirmSensitiveEntryAccess(
      const url::Origin& origin,
      const content::PathInfo& path,
      HandleType handle_type,
      UserAction user_action,
      content::GlobalRenderFrameHostId frame_id,
      base::OnceCallback<void(SensitiveEntryResult)> callback) override;

  void PerformAfterWriteChecks(
      std::unique_ptr<content::FileSystemAccessWriteItem> item,
      content::GlobalRenderFrameHostId frame_id,
      base::OnceCallback<void(AfterWriteCheckResult)> callback) override;

  bool IsFileTypeDangerous(const base::FilePath& path,
                           const url::Origin& origin) override;
  base::expected<void, std::string> CanShowFilePicker(
      content::RenderFrameHost* rfh) override;
  bool CanObtainReadPermission(const url::Origin& origin) override;
  bool CanObtainWritePermission(const url::Origin& origin) override;

  void SetLastPickedDirectory(const url::Origin& origin,
                              const std::string& id,
                              const content::PathInfo& path) override;

  content::PathInfo GetLastPickedDirectory(const url::Origin& origin,
                                           const std::string& id) override;

  base::FilePath GetWellKnownDirectoryPath(
      blink::mojom::WellKnownDirectory directory,
      const url::Origin& origin) override;

  std::u16string GetPickerTitle(
      const blink::mojom::FilePickerOptionsPtr& options) override;

  void NotifyEntryMoved(const url::Origin& origin,
                        const content::PathInfo& old_path,
                        const content::PathInfo& new_path) override;
  void NotifyEntryModified(const url::Origin& origin,
                           const content::PathInfo& path) override;
  void NotifyEntryRemoved(const url::Origin& origin,
                          const content::PathInfo& path) override;

  void OnFileCreatedFromShowSaveFilePicker(
      const GURL& file_picker_binding_context,
      const storage::FileSystemURL& url) override;

  void CheckPathsAgainstEnterprisePolicy(
      std::vector<content::PathInfo> entries,
      content::GlobalRenderFrameHostId frame_id,
      EntriesAllowedByEnterprisePolicyCallback callback) override;

  enum class Access { kRead, kWrite, kReadWrite };

  enum class RequestType { kNewPermission, kRestorePermissions };

  void RevokeActiveGrants(const url::Origin& origin,
                          const base::FilePath& file_path = base::FilePath());

  bool OriginHasReadAccess(const url::Origin& origin);
  bool OriginHasWriteAccess(const url::Origin& origin);

  // Called by FileSystemAccessWebContentsHelper when a top-level frame was
  // navigated away from `origin` to some other origin.
  void NavigatedAwayFromOrigin(const url::Origin& origin);

  content::BrowserContext* browser_context() const { return browser_context_; }

 protected:
  SEQUENCE_CHECKER(sequence_checker_);

 private:
  class PermissionGrantImpl;

  void PermissionGrantDestroyed(PermissionGrantImpl* grant);

  // Restores the read permission for `path` if it was previously downgraded,
  // e.g. by a `remove()` call.
  void MaybeRestoreReadPermission(const url::Origin& origin,
                                  const base::FilePath& path);

  void CheckShouldBlockAccessToPathAndReply(
      base::FilePath path,
      HandleType handle_type,
      std::vector<ChromeFileSystemAccessPermissionContext::BlockPathRule>
          extra_rules,
      base::OnceCallback<void(bool)> callback,
      ChromeFileSystemAccessPermissionContext::BlockPathRules block_path_rules);

  void CheckPathAgainstBlocklist(const content::PathInfo& path,
                                 HandleType handle_type,
                                 base::OnceCallback<void(bool)> callback);
  void DidCheckPathAgainstBlocklist(const url::Origin& origin,
                                    const content::PathInfo& path,
                                    HandleType handle_type,
                                    UserAction user_action,
                                    content::GlobalRenderFrameHostId frame_id,
                                    bool should_block);

  void RunRestrictedPathCallback(const base::FilePath& file_path,
                                 SensitiveEntryResult result);

  void OnRestrictedPathResult(const base::FilePath& file_path,
                              gin::Arguments* args);

  void MaybeEvictEntries(base::DictValue& dict);

  void CleanupPermissions(const url::Origin& origin);

  bool AncestorHasActivePermission(const url::Origin& origin,
                                   const base::FilePath& path,
                                   GrantType grant_type) const;

  void ResetBlockPaths();
  void UpdateBlockPaths(
      std::unique_ptr<ChromeFileSystemAccessPermissionContext::BlockPathRules>
          block_path_rules);

  base::WeakPtr<FileSystemAccessPermissionContext> GetWeakPtr();

  const raw_ptr<content::BrowserContext, DanglingUntriaged> browser_context_;

  struct OriginState;
  std::map<url::Origin, OriginState> active_permissions_map_;

  // Number of custom IDs an origin can specify.
  size_t max_ids_per_origin_ = 32u;

  const raw_ptr<const base::Clock> clock_;

  std::map<url::Origin, base::DictValue> id_pathinfo_map_;

  std::map<base::FilePath, base::OnceCallback<void(SensitiveEntryResult)>>
      callback_map_;

  std::unique_ptr<ChromeFileSystemAccessPermissionContext::BlockPathRules>
      block_path_rules_;
  bool is_block_path_rules_init_complete_ = false;
  std::vector<base::CallbackListSubscription> block_rules_check_subscription_;
  base::OnceCallbackList<void(
      ChromeFileSystemAccessPermissionContext::BlockPathRules)>
      block_rules_check_callbacks_;

  base::WeakPtrFactory<FileSystemAccessPermissionContext> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_FILE_SYSTEM_ACCESS_PERMISSION_CONTEXT_H_
