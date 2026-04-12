// Copyright (c) 2026 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/linux_window_controls_overlay_utils.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace electron::linux_window_controls_overlay {

TEST(LinuxWindowControlsOverlayUtilsTest, PreservesResolvedOrder) {
  const ButtonOrder order = ResolveButtonOrder(
      {views::FrameButton::kClose},
      {views::FrameButton::kMinimize, views::FrameButton::kMaximize});

  EXPECT_EQ(order.leading_buttons,
            (std::vector<views::FrameButton>{views::FrameButton::kClose}));
  EXPECT_EQ(order.trailing_buttons,
            (std::vector<views::FrameButton>{
                views::FrameButton::kMinimize, views::FrameButton::kMaximize}));
}

TEST(LinuxWindowControlsOverlayUtilsTest, FallsBackWhenOrderingUnavailable) {
  const ButtonOrder order = ResolveButtonOrder(
      std::vector<views::FrameButton>{}, std::vector<views::FrameButton>{});

  EXPECT_TRUE(order.leading_buttons.empty());
  EXPECT_EQ(order.trailing_buttons,
            (std::vector<views::FrameButton>{
                views::FrameButton::kMinimize, views::FrameButton::kMaximize,
                views::FrameButton::kClose}));
}

TEST(LinuxWindowControlsOverlayUtilsTest, ResolvesUpdatedOrderOnRelayout) {
  const ButtonOrder first_order = ResolveButtonOrder(
      std::vector<views::FrameButton>{}, std::vector<views::FrameButton>{});
  const ButtonOrder updated_order = ResolveButtonOrder(
      {views::FrameButton::kClose},
      {views::FrameButton::kMaximize, views::FrameButton::kMinimize});

  EXPECT_EQ(first_order.trailing_buttons,
            (std::vector<views::FrameButton>{
                views::FrameButton::kMinimize, views::FrameButton::kMaximize,
                views::FrameButton::kClose}));
  EXPECT_EQ(updated_order.leading_buttons,
            (std::vector<views::FrameButton>{views::FrameButton::kClose}));
  EXPECT_EQ(updated_order.trailing_buttons,
            (std::vector<views::FrameButton>{
                views::FrameButton::kMaximize, views::FrameButton::kMinimize}));
}

TEST(LinuxWindowControlsOverlayUtilsTest, PreservesOverlayGeometry) {
  const gfx::Rect overlay_bounds =
      GetOverlayBounds(gfx::Rect(12, 24, 400, 50),
                       /*leading_reserved_width=*/0,
                       /*trailing_reserved_width=*/90,
                       /*overlay_height=*/31);

  EXPECT_EQ(overlay_bounds, gfx::Rect(0, 0, 310, 31));
}

TEST(LinuxWindowControlsOverlayUtilsTest, ReservesBothButtonBanks) {
  const gfx::Rect overlay_bounds =
      GetOverlayBounds(gfx::Rect(12, 24, 400, 50),
                       /*leading_reserved_width=*/50,
                       /*trailing_reserved_width=*/90,
                       /*overlay_height=*/31);

  EXPECT_EQ(overlay_bounds, gfx::Rect(50, 0, 260, 31));
}

}  // namespace electron::linux_window_controls_overlay
