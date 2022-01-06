// Copyright (c) 2021 Ryan Gonzalez.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
// Portions of this file are sourced from
// chrome/browser/ui/views/frame/browser_desktop_window_tree_host_linux.h,
// Copyright (c) 2019 The Chromium Authors,
// which is governed by a BSD-style license

#ifndef SHELL_BROWSER_UI_ELECTRON_DESKTOP_WINDOW_TREE_HOST_LINUX_H_
#define SHELL_BROWSER_UI_ELECTRON_DESKTOP_WINDOW_TREE_HOST_LINUX_H_

#include "base/scoped_observation.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/client_frame_view_linux.h"
#include "ui/native_theme/native_theme_observer.h"
#include "ui/views/linux_ui/device_scale_factor_observer.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"

namespace electron {

class ElectronDesktopWindowTreeHostLinux
    : public views::DesktopWindowTreeHostLinux,
      public ui::NativeThemeObserver,
      public views::DeviceScaleFactorObserver {
 public:
  ElectronDesktopWindowTreeHostLinux(
      NativeWindowViews* native_window_view,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~ElectronDesktopWindowTreeHostLinux() override;

  // disable copy
  ElectronDesktopWindowTreeHostLinux(
      const ElectronDesktopWindowTreeHostLinux&) = delete;
  ElectronDesktopWindowTreeHostLinux& operator=(
      const ElectronDesktopWindowTreeHostLinux&) = delete;

  bool SupportsClientFrameShadow() const;

 protected:
  // views::DesktopWindowTreeHostLinuxImpl:
  void OnWidgetInitDone() override;

  // ui::PlatformWindowDelegate
  void OnBoundsChanged(const BoundsChange& change) override;
  void OnWindowStateChanged(ui::PlatformWindowState old_state,
                            ui::PlatformWindowState new_state) override;

  // ui::NativeThemeObserver:
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override;

  // views::OnDeviceScaleFactorChanged:
  void OnDeviceScaleFactorChanged() override;

 private:
  void UpdateFrameHints();
  void UpdateClientDecorationHints(ClientFrameViewLinux* view);

  NativeWindowViews* native_window_view_;  // weak ref

  base::ScopedObservation<ui::NativeTheme, ui::NativeThemeObserver>
      theme_observation_{this};
  base::ScopedObservation<views::LinuxUI,
                          views::DeviceScaleFactorObserver,
                          &views::LinuxUI::AddDeviceScaleFactorObserver,
                          &views::LinuxUI::RemoveDeviceScaleFactorObserver>
      scale_observation_{this};
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_ELECTRON_DESKTOP_WINDOW_TREE_HOST_LINUX_H_
