// Copyright (c) 2024 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_ELECTRON_FILE_SYSTEM_ACCESS_PERMISSION_CONTEXT_H_
#define ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_ELECTRON_FILE_SYSTEM_ACCESS_PERMISSION_CONTEXT_H_

#include "shell/browser/file_system_access/electron_file_system_access_permission_context.h"

#include <string>

#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/file_system_access_permission_context.h"

class GURL;

namespace storage {
class FileSystemURL;
}  // namespace storage

class ElectronFileSystemAccessPermissionContext
    : public KeyedService,
      public content::FileSystemAccessPermissionContext {
 public:
  enum class GrantType { kRead, kWrite };

  explicit ElectronFileSystemAccessPermissionContext(
      content::BrowserContext* browser_context);
  ElectronFileSystemAccessPermissionContext(
      const ElectronFileSystemAccessPermissionContext&) = delete;
  ElectronFileSystemAccessPermissionContext& operator=(
      const ElectronFileSystemAccessPermissionContext&) = delete;
  ~ElectronFileSystemAccessPermissionContext() override;

  // content::FileSystemAccessPermissionContext:
  scoped_refptr<content::FileSystemAccessPermissionGrant>
  GetReadPermissionGrant(const url::Origin& origin,
                         const base::FilePath& path,
                         HandleType handle_type,
                         UserAction user_action) override;

  scoped_refptr<content::FileSystemAccessPermissionGrant>
  GetWritePermissionGrant(const url::Origin& origin,
                          const base::FilePath& path,
                          HandleType handle_type,
                          UserAction user_action) override;

  void ConfirmSensitiveEntryAccess(
      const url::Origin& origin,
      PathType path_type,
      const base::FilePath& path,
      HandleType handle_type,
      UserAction user_action,
      content::GlobalRenderFrameHostId frame_id,
      base::OnceCallback<void(SensitiveEntryResult)> callback) override;

  void PerformAfterWriteChecks(
      std::unique_ptr<content::FileSystemAccessWriteItem> item,
      content::GlobalRenderFrameHostId frame_id,
      base::OnceCallback<void(AfterWriteCheckResult)> callback) override;

  bool CanObtainReadPermission(const url::Origin& origin) override;
  bool CanObtainWritePermission(const url::Origin& origin) override;

  void SetLastPickedDirectory(const url::Origin& origin,
                              const std::string& id,
                              const base::FilePath& path,
                              const PathType type) override;

  PathInfo GetLastPickedDirectory(const url::Origin& origin,
                                  const std::string& id) override;

  base::FilePath GetWellKnownDirectoryPath(
      blink::mojom::WellKnownDirectory directory,
      const url::Origin& origin) override;

  std::u16string GetPickerTitle(
      const blink::mojom::FilePickerOptionsPtr& options) override;

  void NotifyEntryMoved(const url::Origin& origin,
                        const base::FilePath& old_path,
                        const base::FilePath& new_path) override;

  void OnFileCreatedFromShowSaveFilePicker(
      const GURL& file_picker_binding_context,
      const storage::FileSystemURL& url) override;

  enum class Access { kRead, kWrite, kReadWrite };

  enum class RequestType { kNewPermission, kRestorePermissions };

  struct FileRequestData {
    FileRequestData(const base::FilePath& path,
                    HandleType handle_type,
                    Access access)
        : path(path), handle_type(handle_type), access(access) {}
    ~FileRequestData() = default;
    FileRequestData(FileRequestData&&) = default;
    FileRequestData(const FileRequestData&) = default;
    FileRequestData& operator=(FileRequestData&&) = default;
    FileRequestData& operator=(const FileRequestData&) = default;

    base::FilePath path;
    HandleType handle_type;
    Access access;
  };

  struct RequestData {
    RequestData(RequestType request_type,
                const url::Origin& origin,
                const std::vector<FileRequestData>& file_request_data);
    ~RequestData();
    RequestData(RequestData&&);
    RequestData(const RequestData&);
    RequestData& operator=(RequestData&&) = default;
    RequestData& operator=(const RequestData&) = default;

    RequestType request_type;
    url::Origin origin;
    std::vector<FileRequestData> file_request_data;
  };

  struct Grants {
    Grants();
    ~Grants();
    Grants(Grants&&);
    Grants& operator=(Grants&&);

    std::vector<base::FilePath> file_read_grants;
    std::vector<base::FilePath> file_write_grants;
    std::vector<base::FilePath> directory_read_grants;
    std::vector<base::FilePath> directory_write_grants;
  };

  void RevokeGrant(const url::Origin& origin,
                   const base::FilePath& file_path = base::FilePath());

  bool OriginHasReadAccess(const url::Origin& origin);
  bool OriginHasWriteAccess(const url::Origin& origin);

  content::BrowserContext* browser_context() const { return browser_context_; }

 protected:
  SEQUENCE_CHECKER(sequence_checker_);

 private:
  class PermissionGrantImpl;

  void PermissionGrantDestroyed(PermissionGrantImpl* grant);

  void CheckPathAgainstBlocklist(PathType path_type,
                                 const base::FilePath& path,
                                 HandleType handle_type,
                                 base::OnceCallback<void(bool)> callback);
  void DidCheckPathAgainstBlocklist(
      const url::Origin& origin,
      const base::FilePath& path,
      HandleType handle_type,
      UserAction user_action,
      content::GlobalRenderFrameHostId frame_id,
      base::OnceCallback<void(SensitiveEntryResult)> callback,
      bool should_block);

  void CleanupPermissions(const url::Origin& origin);

  bool AncestorHasActivePermission(const url::Origin& origin,
                                   const base::FilePath& path,
                                   GrantType grant_type) const;

  base::WeakPtr<ElectronFileSystemAccessPermissionContext> GetWeakPtr();

  const raw_ptr<content::BrowserContext, DanglingUntriaged> browser_context_;

  struct OriginState;
  std::map<url::Origin, OriginState> active_permissions_map_;

  base::WeakPtrFactory<ElectronFileSystemAccessPermissionContext> weak_factory_{
      this};
};

#endif  // ELECTRON_SHELL_BROWSER_FILE_SYSTEM_ACCESS_ELECTRON_FILE_SYSTEM_ACCESS_PERMISSION_CONTEXT_H_
