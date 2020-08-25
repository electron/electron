// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/platform_util.h"

#include <utility>

#include "base/bind.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/common/platform_util_internal.h"

namespace platform_util {

void TrashItemOnBlockingThread(
    const base::FilePath& full_path,
    base::OnceCallback<void(bool, const std::string&)> callback) {
  std::string error;
  bool success = internal::PlatformTrashItem(full_path, &error);
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), success, error));
}

void TrashItem(const base::FilePath& full_path,
               base::OnceCallback<void(bool, const std::string&)> callback) {
  // XXX: is continue_on_shutdown right?
  base::ThreadPool::PostTask(FROM_HERE,
                             {base::MayBlock(), base::WithBaseSyncPrimitives(),
                              base::TaskPriority::USER_BLOCKING,
                              base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
                             base::BindOnce(&TrashItemOnBlockingThread,
                                            full_path, std::move(callback)));
}

}  // namespace platform_util
