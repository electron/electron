// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/accelerator_util.h"

#include "base/memory/raw_ref.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace accelerator_util {

TEST(AcceleratorUtilTest, StringToAccelerator) {
  struct {
    const std::string description;
    bool expected_success;
  } keys[] = {
      {"♫♫♫♫♫♫♫", false},   {"Cmd+Plus", true}, {"Ctrl+Space", true},
      {"CmdOrCtrl", false}, {"Alt+Tab", true},  {"AltGr+Backspace", true},
      {"Super+Esc", true},  {"Super+X", true},  {"Shift+1", true},
  };

  for (const auto& key : keys) {
    // Initialize empty-but-not-null accelerator
    ui::Accelerator out = ui::Accelerator(ui::VKEY_UNKNOWN, ui::EF_NONE);
    bool success = StringToAccelerator(key.description, &out);
    EXPECT_EQ(success, key.expected_success);
  }
}

}  // namespace accelerator_util
