// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/atom_process_singleton.h"

AtomProcessSingleton::AtomProcessSingleton(
    const base::FilePath& user_data_dir,
    const ProcessSingleton::NotificationCallback& notification_callback)
    : startup_lock_(notification_callback),
      process_singleton_(user_data_dir,
                         startup_lock_.AsNotificationCallback()) {
}

AtomProcessSingleton::~AtomProcessSingleton() {
}

ProcessSingleton::NotifyResult
    AtomProcessSingleton::NotifyOtherProcessOrCreate() {
  return process_singleton_.NotifyOtherProcessOrCreate();
}

void AtomProcessSingleton::Cleanup() {
  process_singleton_.Cleanup();
}

void AtomProcessSingleton::Unlock() {
  startup_lock_.Unlock();
}
