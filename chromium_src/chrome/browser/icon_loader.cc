// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/icon_loader.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

// static
IconLoader* IconLoader::Create(const base::FilePath& file_path,
                               IconSize size,
                               IconLoadedCallback callback) {
  return new IconLoader(file_path, size, callback);
}

void IconLoader::Start() {
  target_task_runner_ = base::ThreadTaskRunnerHandle::Get();

  base::PostTaskWithTraits(
      FROM_HERE, traits(),
      base::BindOnce(&IconLoader::ReadGroup, base::Unretained(this)));
}

IconLoader::IconLoader(const base::FilePath& file_path,
                       IconSize size,
                       IconLoadedCallback callback)
    : file_path_(file_path), icon_size_(size), callback_(callback) {}

IconLoader::~IconLoader() {}

void IconLoader::ReadGroup() {
  group_ = GroupForFilepath(file_path_);
  GetReadIconTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&IconLoader::ReadIcon, base::Unretained(this)));
}
