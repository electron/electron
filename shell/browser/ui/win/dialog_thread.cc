// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/dialog_thread.h"

#include "base/win/scoped_com_initializer.h"

namespace dialog_thread {

// Creates a SingleThreadTaskRunner to run a shell dialog on. Each dialog
// requires its own dedicated single-threaded sequence otherwise in some
// situations where a singleton owns a single instance of this object we can
// have a situation where a modal dialog in one window blocks the appearance
// of a modal dialog in another.
TaskRunner CreateDialogTaskRunner() {
  return CreateCOMSTATaskRunner(
      {base::ThreadPool(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN, base::MayBlock()},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED);
}

}  // namespace dialog_thread
