// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/cleaned_up_at_exit.h"

#include "base/containers/flat_set.h"
#include "base/no_destructor.h"

namespace gin_helper {

base::flat_set<CleanedUpAtExit*>& GetDoomed() {
  static base::NoDestructor<base::flat_set<CleanedUpAtExit*>> doomed;
  return *doomed;
}
CleanedUpAtExit::CleanedUpAtExit() {
  GetDoomed().insert(this);
}
CleanedUpAtExit::~CleanedUpAtExit() {
  GetDoomed().erase(this);
}

// static
void CleanedUpAtExit::DoCleanup() {
  auto& doomed = GetDoomed();
  while (!doomed.empty()) {
    auto iter = doomed.begin();
    delete *iter;
    // It removed itself from the list.
  }
}

}  // namespace gin_helper
