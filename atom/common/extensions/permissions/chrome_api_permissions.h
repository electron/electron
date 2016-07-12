// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_EXTENSIONS_PERMISSIONS_CHROME_API_PERMISSIONS_H_
#define ATOM_COMMON_EXTENSIONS_PERMISSIONS_CHROME_API_PERMISSIONS_H_

#include <vector>

#include "base/compiler_specific.h"
#include "extensions/common/permissions/permissions_provider.h"

namespace extensions {

// this class has to be extensions::ChromeAPIPermissions because
// InitInfo is a private member of APIPermissionInfo and only
// ExtensionAPIPermission and ChromeAPIPermission are listed
// as friend classes
class ChromeAPIPermissions : public PermissionsProvider {
 public:
  std::vector<APIPermissionInfo*> GetAllPermissions() const override;
  std::vector<PermissionsProvider::AliasInfo> GetAllAliases() const override;
};

}  // namespace extensions

#endif  // ATOM_COMMON_EXTENSIONS_PERMISSIONS_CHROME_API_PERMISSIONS_H_
