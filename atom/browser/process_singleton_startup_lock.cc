// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/process_singleton_startup_lock.h"

#include "base/bind.h"
#include "base/logging.h"

ProcessSingletonStartupLock::ProcessSingletonStartupLock(
    const ProcessSingleton::NotificationCallback& original_callback)
    : locked_(true),
      original_callback_(original_callback) {}

ProcessSingletonStartupLock::~ProcessSingletonStartupLock() {}

ProcessSingleton::NotificationCallback
ProcessSingletonStartupLock::AsNotificationCallback() {
  return base::Bind(&ProcessSingletonStartupLock::NotificationCallbackImpl,
                    base::Unretained(this));
}

void ProcessSingletonStartupLock::Unlock() {
  DCHECK(CalledOnValidThread());
  locked_ = false;

  // Replay the command lines of the messages which were received while the
  // ProcessSingleton was locked. Only replay each message once.
  std::set<DelayedStartupMessage> replayed_messages;
  for (std::vector<DelayedStartupMessage>::const_iterator it =
           saved_startup_messages_.begin();
       it != saved_startup_messages_.end(); ++it) {
    if (replayed_messages.find(*it) != replayed_messages.end())
      continue;
    original_callback_.Run(base::CommandLine(it->first), it->second);
    replayed_messages.insert(*it);
  }
  saved_startup_messages_.clear();
}

bool ProcessSingletonStartupLock::NotificationCallbackImpl(
    const base::CommandLine& command_line,
    const base::FilePath& current_directory) {
  if (locked_) {
    // If locked, it means we are not ready to process this message because
    // we are probably in a first run critical phase.
    saved_startup_messages_.push_back(
        std::make_pair(command_line.argv(), current_directory));
    return true;
  } else {
    return original_callback_.Run(command_line, current_directory);
  }
}
