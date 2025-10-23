// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/cleaned_up_at_exit.h"

#include <algorithm>
#include <vector>

#include "base/no_destructor.h"

namespace gin_helper {

namespace {

std::vector<CleanedUpAtExit*>& GetDoomed() {
  static base::NoDestructor<std::vector<CleanedUpAtExit*>> doomed;
  return *doomed;
}

}  // namespace

CleanedUpAtExit::CleanedUpAtExit() {
  GetDoomed().emplace_back(this);
}
CleanedUpAtExit::~CleanedUpAtExit() {
  std::erase(GetDoomed(), this);
}

void CleanedUpAtExit::WillBeDestroyed() {}

// static
void CleanedUpAtExit::DoCleanup() {
  auto& doomed = GetDoomed();
  while (!doomed.empty()) {
    CleanedUpAtExit* next = doomed.back();
    next->WillBeDestroyed();
    delete next;
  }
}

}  // namespace gin_helper
