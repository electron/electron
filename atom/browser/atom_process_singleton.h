// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_PROCESS_SINGLETON_H_
#define ATOM_BROWSER_ATOM_PROCESS_SINGLETON_H_

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "atom/browser/process_singleton.h"
#include "atom/browser/process_singleton_startup_lock.h"

// Composes a basic ProcessSingleton with ProcessSingletonStartupLock
class AtomProcessSingleton {
 public:
  AtomProcessSingleton(
      const base::FilePath& user_data_dir,
      const ProcessSingleton::NotificationCallback& notification_callback);
      
  ~AtomProcessSingleton();

  // Notify another process, if available. Otherwise sets ourselves as the
  // singleton instance. Returns PROCESS_NONE if we became the singleton
  // instance. Callers are guaranteed to either have notified an existing
  // process or have grabbed the singleton (unless the profile is locked by an
  // unreachable process).
  ProcessSingleton::NotifyResult NotifyOtherProcessOrCreate();

  // Clear any lock state during shutdown.
  void Cleanup();

  // Executes previously queued command-line invocations and allows future
  // invocations to be executed immediately.
  // This only has an effect the first time it is called.
  void Unlock();

 private:
  // We compose these two locks with the client-supplied notification callback.
  // First |modal_dialog_lock_| will discard any notifications that arrive while
  // a modal dialog is active. Otherwise, it will pass the notification to
  // |startup_lock_|, which will queue notifications until |Unlock()| is called.
  // Notifications passing through both locks are finally delivered to our
  // client.
  ProcessSingletonStartupLock startup_lock_;

  // The basic ProcessSingleton
  ProcessSingleton process_singleton_;

  DISALLOW_COPY_AND_ASSIGN(AtomProcessSingleton);
};

#endif  // ATOM_BROWSER_ATOM_PROCESS_SINGLETON_H_
