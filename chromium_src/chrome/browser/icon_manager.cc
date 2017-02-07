// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/icon_manager.h"

#include <memory>
#include <tuple>

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/task_runner.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

struct IconManager::ClientRequest {
  IconRequestCallback callback;
  base::FilePath file_path;
  IconLoader::IconSize size;
};

// static
IconManager* IconManager::GetInstance() {
  return base::Singleton<IconManager>::get();
}

IconManager::IconManager() {}

IconManager::~IconManager() {
  base::STLDeleteValues(&icon_cache_);
}

gfx::Image* IconManager::LookupIconFromFilepath(const base::FilePath& file_name,
                                                IconLoader::IconSize size) {
  GroupMap::iterator it = group_cache_.find(file_name);
  if (it != group_cache_.end())
    return LookupIconFromGroup(it->second, size);

  return NULL;
}

gfx::Image* IconManager::LookupIconFromGroup(const IconGroupID& group,
                                             IconLoader::IconSize size) {
  IconMap::iterator it = icon_cache_.find(CacheKey(group, size));
  if (it != icon_cache_.end())
    return it->second;

  return nullptr;
}

void IconManager::LoadIcon(const base::FilePath& file_name,
                           IconLoader::IconSize size,
                           const IconRequestCallback& callback) {
  IconLoader* loader = new IconLoader(file_name, size, this);
  loader->AddRef();
  loader->Start();

  ClientRequest client_request = {callback, file_name, size};
  requests_[loader] = client_request;
}

// IconLoader::Delegate implementation -----------------------------------------

bool IconManager::OnGroupLoaded(IconLoader* loader, const IconGroupID& group) {
  ClientRequests::iterator rit = requests_.find(loader);
  if (rit == requests_.end()) {
    NOTREACHED();
    return false;
  }

  gfx::Image* result = LookupIconFromGroup(group, rit->second.size);
  if (!result) {
    return false;
  }

  return OnImageLoaded(loader, result, group);
}

bool IconManager::OnImageLoaded(IconLoader* loader,
                                gfx::Image* result,
                                const IconGroupID& group) {
  ClientRequests::iterator rit = requests_.find(loader);

  // Balances the AddRef() in LoadIcon().
  loader->Release();

  // Look up our client state.
  if (rit == requests_.end()) {
    NOTREACHED();
    return false;  // Return false to indicate result should be deleted.
  }

  const ClientRequest& client_request = rit->second;

  // Cache the bitmap. Watch out: |result| may be NULL to indicate a current
  // failure. We assume that if we have an entry in |icon_cache_|
  // it must not be NULL.
  CacheKey key(group, client_request.size);
  IconMap::iterator it = icon_cache_.find(key);
  if (it != icon_cache_.end()) {
    if (!result) {
      delete it->second;
      icon_cache_.erase(it);
    } else if (result != it->second) {
      it->second->SwapRepresentations(result);
      delete result;
      result = it->second;
    }
  } else if (result) {
    icon_cache_[key] = result;
  }

  group_cache_[client_request.file_path] = group;

  // Inform our client that the request has completed.
  client_request.callback.Run(result);
  requests_.erase(rit);

  return true;  // Indicates we took ownership of result.
}

IconManager::CacheKey::CacheKey(const IconGroupID& group,
                                IconLoader::IconSize size)
    : group(group), size(size) {}

bool IconManager::CacheKey::operator<(const CacheKey& other) const {
  return std::tie(group, size) < std::tie(other.group, other.size);
}
