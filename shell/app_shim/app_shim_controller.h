// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_SHIM_APP_SHIM_CONTROLLER_H_
#define ELECTRON_SHELL_APP_SHIM_APP_SHIM_CONTROLLER_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "electron/shell/common/app_shim.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/system/isolated_connection.h"
#include "shell/app_shim/app_shim_menu.h"

class GURL;

namespace display {
class ScopedNativeScreen;
}  // namespace display

@class ElectronAppShimDelegate;

@class ElectronMenuController;

namespace electron {

// Runs an app shim process: finds (or starts) the host Electron app, connects
// to it, and then serves the windows the host assigns to this process.
class AppShimController : public mojom::AppShim, private AppShimMenu::Client {
 public:
  explicit AppShimController(const base::FilePath& host_bundle_path);
  ~AppShimController() override;

  // The process's one controller, or null before it is made.
  static AppShimController* Get();

  AppShimController(const AppShimController&) = delete;
  AppShimController& operator=(const AppShimController&) = delete;

  // From the NSApplication delegate.
  void OnReopen(bool has_visible_windows);
  void OnOpenFiles(std::vector<base::FilePath> files);
  void OnOpenUrls(std::vector<GURL> urls);
  void OnDidBecomeActive();
  void OnNewWindowForTab(uint64_t window_id);
  // From -[NSApplication terminate:]. False when the host was asked instead;
  // Quit(), or no answer in time, then terminates the app.
  bool ShouldTerminate();
  // From the NSApplication. Each mode is asked for at most once.
  void EnableAccessibilitySupport(mojom::AppShimAccessibilityMode mode);

 private:
  // The name the host publishes its bootstrap server under.
  std::string ServerName() const;
  bool IsHostRunning() const;
  void LaunchHost();
  void PollForHost();
  // Hands the host, whose bootstrap server `server_endpoint` is, one end of
  // a new channel; invalid when that fails.
  static mojo::PlatformChannelEndpoint ConnectToHost(
      mojo::PlatformChannelEndpoint server_endpoint);
  // The host is running another build of Electron than this process loaded.
  // Tells the host, which tells the person, and quits.
  void OnBuildMismatch(mojo::PlatformChannelEndpoint server_endpoint);
  void OnConnected(mojo::PlatformChannelEndpoint endpoint);
  void OnShimConnectedResponse(
      mojo::PendingReceiver<mojom::AppShim> app_shim_receiver);
  void Terminate();
  void OnQuitAnswerTimeout();
  void SetUpMenu();

  // AppShimMenu::Client:
  void OnMenuItemActivated(const std::vector<uint32_t>& path,
                           int command_id,
                           int event_flags) override;
  void OnMenuWillShow() override;
  void OnMenuWillClose() override;

  // mojom::AppShim:
  void CreateRemoteCocoaApplication(
      mojo::PendingAssociatedReceiver<remote_cocoa::mojom::Application>
          application) override;
  void SetBadgeLabel(const std::string& label) override;
  void SetUserAttention(mojom::AppShimAttentionType type) override;
  void Quit() override;
  void CancelQuit() override;
  void SetApplicationMenu(
      std::optional<std::vector<mojom::AppShimMenuItemPtr>> items) override;
  void InitWindowChrome(uint64_t window_id,
                        mojom::AppShimWindowChromePtr chrome) override;
  void SetWindowButtonVisibility(uint64_t window_id, bool visible) override;
  void SetWindowButtonPosition(
      uint64_t window_id,
      const std::optional<gfx::Point>& position) override;
  void SetWindowVibrancy(uint64_t window_id,
                         const std::string& type,
                         int32_t duration_ms,
                         int32_t state) override;
  void PerformWindowTabAction(uint64_t window_id,
                              mojom::AppShimTabAction action) override;
  void AddTabbedWindow(uint64_t window_id, uint64_t tabbed_window_id) override;
  void PreviewFile(uint64_t window_id,
                   const base::FilePath& path,
                   const std::string& display_name) override;
  void CloseFilePreview(uint64_t window_id) override;
  void ShowMessageBox(uint64_t window_id,
                      mojom::AppShimMessageBoxOptionsPtr options,
                      ShowMessageBoxCallback callback) override;
  void CloseMessageBox(int32_t id) override;
  void ShowOpenDialog(uint64_t window_id,
                      mojom::AppShimFileDialogOptionsPtr options,
                      ShowOpenDialogCallback callback) override;
  void ShowSaveDialog(uint64_t window_id,
                      mojom::AppShimFileDialogOptionsPtr options,
                      ShowSaveDialogCallback callback) override;

  const base::FilePath host_bundle_path_;
  ElectronAppShimDelegate* __strong delegate_;
  // The windows this process hosts ask it about the displays.
  std::unique_ptr<display::ScopedNativeScreen> screen_;

  base::TimeTicks poll_deadline_;
  bool launched_host_ = false;
  // The host's menu, while it has given the shim one. The controller only
  // refers to the menu's model.
  std::unique_ptr<AppShimMenu> menu_;
  ElectronMenuController* __strong menu_controller_;

  bool terminating_ = false;
  // The host has been asked whether the shim may quit.
  bool waiting_for_quit_answer_ = false;
  base::OneShotTimer quit_answer_timer_;
  bool asked_for_partial_accessibility_ = false;
  bool asked_for_complete_accessibility_ = false;
  NSInteger attention_request_id_ = 0;

  mojo::IsolatedConnection bootstrap_connection_;
  mojo::Remote<mojom::AppShimHostBootstrap> host_bootstrap_;
  mojo::Receiver<mojom::AppShim> shim_receiver_{this};
  mojo::Remote<mojom::AppShimHost> host_;
  // Calls on `host_` made before the host binds this are delivered once it
  // does.
  mojo::PendingReceiver<mojom::AppShimHost> host_receiver_;

  base::WeakPtrFactory<AppShimController> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_SHIM_APP_SHIM_CONTROLLER_H_
