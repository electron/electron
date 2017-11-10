// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/icon_manager.h"

#include <memory>
#include <tuple>

#include "base/bind.h"
#include "base/task_runner.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace {

void RunCallbackIfNotCanceled(
    const base::CancelableTaskTracker::IsCanceledCallback& is_canceled,
    const IconManager::IconRequestCallback& callback,
    gfx::Image* image) {
  if (is_canceled.Run())
    return;
  callback.Run(image);
}

}  // namespace

IconManager::IconManager() : weak_factory_(this) {}

IconManager::~IconManager() {
}

gfx::Image* IconManager::LookupIconFromFilepath(const base::FilePath& file_path,
                                                IconLoader::IconSize size) {
  auto group_it = group_cache_.find(file_path);
  if (group_it == group_cache_.end())
    return nullptr;

  CacheKey key(group_it->second, size);
  auto icon_it = icon_cache_.find(key);
  if (icon_it == icon_cache_.end())
    return nullptr;

  return icon_it->second.get();
}

base::CancelableTaskTracker::TaskId IconManager::LoadIcon(
    const base::FilePath& file_path,
    IconLoader::IconSize size,
    const IconRequestCallback& callback,
    base::CancelableTaskTracker* tracker) {
  base::CancelableTaskTracker::IsCanceledCallback is_canceled;
  base::CancelableTaskTracker::TaskId id =
      tracker->NewTrackedTaskId(&is_canceled);
  IconRequestCallback callback_runner = base::Bind(
      &RunCallbackIfNotCanceled, is_canceled, callback);

  IconLoader* loader = IconLoader::Create(
      file_path, size,
      base::Bind(&IconManager::OnIconLoaded, weak_factory_.GetWeakPtr(),
                 callback_runner, file_path, size));
  loader->Start();

  return id;
}

void IconManager::OnIconLoaded(IconRequestCallback callback,
                               base::FilePath file_path,
                               IconLoader::IconSize size,
                               std::unique_ptr<gfx::Image> result,
                               const IconLoader::IconGroup& group) {
  // Cache the bitmap. Watch out: |result| may be null, which indicates a
  // failure. We assume that if we have an entry in |icon_cache_| it must not be
  // null.
  CacheKey key(group, size);
  if (result) {
    callback.Run(result.get());
    icon_cache_[key] = std::move(result);
  } else {
    callback.Run(nullptr);
    icon_cache_.erase(key);
  }

  group_cache_[file_path] = group;
}

IconManager::CacheKey::CacheKey(const IconLoader::IconGroup& group,
                                IconLoader::IconSize size)
    : group(group), size(size) {}

bool IconManager::CacheKey::operator<(const CacheKey &other) const {
  return std::tie(group, size) < std::tie(other.group, other.size);
}
