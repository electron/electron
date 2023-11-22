// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/win/electron_desktop_native_widget_aura.h"

#include <utility>

#include "shell/browser/ui/win/electron_desktop_window_tree_host_win.h"
#include "ui/views/corewm/tooltip_controller.h"
#include "ui/wm/public/tooltip_client.h"

namespace electron {

ElectronDesktopNativeWidgetAura::ElectronDesktopNativeWidgetAura(
    NativeWindowViews* native_window_view)
    : views::DesktopNativeWidgetAura(native_window_view->widget()),
      native_window_view_(native_window_view) {
  GetNativeWindow()->SetName("ElectronDesktopNativeWidgetAura");
  // This is to enable the override of OnWindowActivated
  wm::SetActivationChangeObserver(GetNativeWindow(), this);
}

void ElectronDesktopNativeWidgetAura::InitNativeWidget(
    views::Widget::InitParams params) {
  desktop_window_tree_host_ = new ElectronDesktopWindowTreeHostWin(
      native_window_view_,
      static_cast<views::DesktopNativeWidgetAura*>(params.native_widget));
  params.desktop_window_tree_host = desktop_window_tree_host_;
  views::DesktopNativeWidgetAura::InitNativeWidget(std::move(params));
}

#if BUILDFLAG(IS_WIN)
void ElectronDesktopNativeWidgetAura::OnSizeConstraintsChanged() {
  views::DesktopNativeWidgetAura::OnSizeConstraintsChanged();

  // OnSizeConstraintsChanged can remove thick frame depending from
  // resizable state, so add it if needed.
  native_window_view_->UpdateThickFrame();
}
#endif

void ElectronDesktopNativeWidgetAura::Activate() {
  // Activate can cause the focused window to be blurred so only
  // call when the window being activated is visible. This prevents
  // hidden windows from blurring the focused window when created.
  if (IsVisible())
    views::DesktopNativeWidgetAura::Activate();
}

void ElectronDesktopNativeWidgetAura::OnWindowActivated(
    wm::ActivationChangeObserver::ActivationReason reason,
    aura::Window* gained_active,
    aura::Window* lost_active) {
  views::DesktopNativeWidgetAura::OnWindowActivated(reason, gained_active,
                                                    lost_active);
  if (lost_active != nullptr) {
    auto* tooltip_controller = static_cast<views::corewm::TooltipController*>(
        wm::GetTooltipClient(lost_active->GetRootWindow()));

    // This will cause the tooltip to be hidden when a window is deactivated,
    // as it should be.
    // TODO(brenca): Remove this fix when the chromium issue is fixed.
    // crbug.com/724538
    if (tooltip_controller != nullptr)
      tooltip_controller->OnCancelMode(nullptr);
  }
}

}  // namespace electron
