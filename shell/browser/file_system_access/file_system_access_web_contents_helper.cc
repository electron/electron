// Copyright (c) 2024 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/file_system_access/file_system_access_web_contents_helper.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "shell/browser/file_system_access/file_system_access_permission_context.h"
#include "shell/browser/file_system_access/file_system_access_permission_context_factory.h"

FileSystemAccessWebContentsHelper::~FileSystemAccessWebContentsHelper() =
    default;

void FileSystemAccessWebContentsHelper::DidFinishNavigation(
    content::NavigationHandle* navigation) {
  // We only care about top-level navigations that actually committed.
  if (!navigation->IsInPrimaryMainFrame() || !navigation->HasCommitted())
    return;

  auto src_origin =
      url::Origin::Create(navigation->GetPreviousPrimaryMainFrameURL());
  auto dest_origin = url::Origin::Create(navigation->GetURL());

  if (src_origin == dest_origin)
    return;

  // Navigated away from |src_origin|, tell permission context to check if
  // permissions need to be revoked.
  auto* context =
      electron::FileSystemAccessPermissionContextFactory::GetForBrowserContext(
          web_contents()->GetBrowserContext());
  if (context)
    context->NavigatedAwayFromOrigin(src_origin);
}

void FileSystemAccessWebContentsHelper::WebContentsDestroyed() {
  auto src_origin =
      web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();

  // Navigated away from |src_origin|, tell permission context to check if
  // permissions need to be revoked.
  auto* context =
      electron::FileSystemAccessPermissionContextFactory::GetForBrowserContext(
          web_contents()->GetBrowserContext());
  if (context)
    context->NavigatedAwayFromOrigin(src_origin);
}

FileSystemAccessWebContentsHelper::FileSystemAccessWebContentsHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<FileSystemAccessWebContentsHelper>(
          *web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(FileSystemAccessWebContentsHelper);
