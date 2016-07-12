// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#include "atom/common/extensions/permissions/chrome_api_permissions.h"

#include <stddef.h>

#include "base/macros.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/permissions/settings_override_permission.h"
#include "extensions/strings/grit/extensions_strings.h"

namespace extensions {

namespace {

const char kWindowsPermission[] = "windows";

}  // namespace

std::vector<APIPermissionInfo*> ChromeAPIPermissions::GetAllPermissions()
    const {
  APIPermissionInfo::InitInfo permissions_to_register[] = {
    // Register permissions for all extension types.
    {APIPermission::kBackground, "background"},
    {APIPermission::kActiveTab, "activeTab"},
    {APIPermission::kContextMenus, "contextMenus"},
    {APIPermission::kCookie, "cookies"},
    {APIPermission::kManagement, "management"},
    {APIPermission::kNativeMessaging, "nativeMessaging"},
    {APIPermission::kPrivacy, "privacy"},
    {APIPermission::kTab, "tabs"},
    {APIPermission::kContentSettings, "contentSettings"},

    // Full url access permissions.
    {APIPermission::kDebugger, "debugger",
     APIPermissionInfo::kFlagImpliesFullURLAccess |
         APIPermissionInfo::kFlagCannotBeOptional},
    {APIPermission::kDevtools, "devtools",
     APIPermissionInfo::kFlagImpliesFullURLAccess |
         APIPermissionInfo::kFlagCannotBeOptional |
         APIPermissionInfo::kFlagInternal},
    {APIPermission::kPageCapture, "pageCapture",
     APIPermissionInfo::kFlagImpliesFullURLAccess},
  };
  std::vector<APIPermissionInfo*> permissions;

  for (size_t i = 0; i < arraysize(permissions_to_register); ++i)
    permissions.push_back(new APIPermissionInfo(permissions_to_register[i]));
  return permissions;
}

std::vector<PermissionsProvider::AliasInfo>
ChromeAPIPermissions::GetAllAliases() const {
  std::vector<PermissionsProvider::AliasInfo> aliases;
  aliases.push_back(PermissionsProvider::AliasInfo("tabs", kWindowsPermission));
  return aliases;
}

}  // namespace extensions
