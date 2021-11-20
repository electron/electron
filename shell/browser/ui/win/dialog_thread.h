// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_WIN_DIALOG_THREAD_H_
#define ELECTRON_SHELL_BROWSER_UI_WIN_DIALOG_THREAD_H_

#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_thread.h"

namespace dialog_thread {

// Returns the dedicated single-threaded sequence that the dialog will be on.
using TaskRunner = scoped_refptr<base::SingleThreadTaskRunner>;
TaskRunner CreateDialogTaskRunner();

// Runs the |execute| in dialog thread and pass result to |done| in UI thread.
template <typename R>
void Run(base::OnceCallback<R()> execute, base::OnceCallback<void(R)> done) {
  // dialogThread.postTask(() => {
  //   r = execute()
  //   uiThread.postTask(() => {
  //     done(r)
  //   }
  // })
  TaskRunner task_runner = CreateDialogTaskRunner();
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](TaskRunner task_runner, base::OnceCallback<R()> execute,
             base::OnceCallback<void(R)> done) {
            R r = std::move(execute).Run();
            base::PostTask(
                FROM_HERE, {content::BrowserThread::UI},
                base::BindOnce(
                    [](TaskRunner task_runner, base::OnceCallback<void(R)> done,
                       R r) {
                      std::move(done).Run(std::move(r));
                      // Task runner will destroyed automatically after the
                      // scope ends.
                    },
                    std::move(task_runner), std::move(done), std::move(r)));
          },
          std::move(task_runner), std::move(execute), std::move(done)));
}

// Adaptor to handle the |execute| that returns bool.
template <typename R>
void Run(base::OnceCallback<bool(R*)> execute,
         base::OnceCallback<void(bool, R)> done) {
  // run(() => {
  //   result = execute(&value)
  //   return {result, value}
  // }, ({result, value}) => {
  //   done(result, value)
  // })
  struct Result {
    bool result;
    R value;
  };
  Run(base::BindOnce(
          [](base::OnceCallback<bool(R*)> execute) {
            Result r;
            r.result = std::move(execute).Run(&r.value);
            return r;
          },
          std::move(execute)),
      base::BindOnce(
          [](base::OnceCallback<void(bool, R)> done, Result r) {
            std::move(done).Run(r.result, std::move(r.value));
          },
          std::move(done)));
}

}  // namespace dialog_thread

#endif  // ELECTRON_SHELL_BROWSER_UI_WIN_DIALOG_THREAD_H_
