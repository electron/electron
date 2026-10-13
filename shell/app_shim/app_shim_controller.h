// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_SHIM_APP_SHIM_CONTROLLER_H_
#define ELECTRON_SHELL_APP_SHIM_APP_SHIM_CONTROLLER_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "electron/shell/common/app_shim.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/system/isolated_connection.h"

class GURL;

namespace display {
class ScopedNativeScreen;
}  // namespace display

@class ElectronAppShimDelegate;

namespace electron {

// Runs an app shim process: finds (or starts) the host Electron app, connects
// to it, and then serves the windows the host assigns to this process.
class AppShimController : public mojom::AppShim {
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
  // False when the host was asked instead, and will answer with Quit().
  bool ShouldTerminate();
  // From the NSApplication. Each mode is asked for at most once.
  void EnableAccessibilitySupport(mojom::AppShimAccessibilityMode mode);

 private:
  // The name the host publishes its bootstrap server under.
  std::string ServerName() const;
  bool IsHostRunning() const;
  void LaunchHost();
  void PollForHost();
  // Hands the host one end of a new channel; invalid when the host is not
  // listening (yet).
  static mojo::PlatformChannelEndpoint ConnectToHost(
      const std::string& server_name);
  void OnConnected(mojo::PlatformChannelEndpoint endpoint);
  void OnShimConnectedResponse(
      mojo::PendingReceiver<mojom::AppShim> app_shim_receiver);
  void Terminate();
  void SetUpMenu();

  // mojom::AppShim:
  void CreateRemoteCocoaApplication(
      mojo::PendingAssociatedReceiver<remote_cocoa::mojom::Application>
          application) override;
  void SetBadgeLabel(const std::string& label) override;
  void SetUserAttention(mojom::AppShimAttentionType type) override;
  void Quit() override;

  const base::FilePath host_bundle_path_;
  ElectronAppShimDelegate* __strong delegate_;
  // The windows this process hosts ask it about the displays.
  std::unique_ptr<display::ScopedNativeScreen> screen_;

  base::TimeTicks poll_deadline_;
  bool launched_host_ = false;
  bool terminating_ = false;
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
