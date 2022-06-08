// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CLEANED_UP_AT_EXIT_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CLEANED_UP_AT_EXIT_H_

namespace gin_helper {

// Objects of this type will be destroyed immediately prior to disposing the V8
// Isolate. This should only be used for gin::Wrappable objects, whose lifetime
// is otherwise managed by V8.
//
// NB. This is only needed because v8::Global objects that have SetWeak
// finalization callbacks do not have their finalization callbacks invoked at
// Isolate teardown.
class CleanedUpAtExit {
 public:
  CleanedUpAtExit();
  virtual ~CleanedUpAtExit();

  static void DoCleanup();
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CLEANED_UP_AT_EXIT_H_
