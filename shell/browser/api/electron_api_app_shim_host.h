// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_APP_SHIM_HOST_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_APP_SHIM_HOST_H_

#include <mach/message.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "electron/shell/common/app_shim.mojom.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/native_peer.h"

class GURL;

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace remote_cocoa {
class ApplicationHost;
}  // namespace remote_cocoa

namespace electron {

class AppShimHostBootstrap;

namespace api {

class Menu;

// A connected app shim: another app, with a Dock tile of its own, that shows
// windows on this app's behalf. Windows created with it as their `appShim`
// option are NSWindows of the shim's process.
class AppShimHost final : public gin::Wrappable<AppShimHost>,
                          public gin_helper::EventEmitterMixin<AppShimHost> {
 public:
  // Takes over a shim's connection once its hello has been answered.
  static AppShimHost* Create(
      v8::Isolate* isolate,
      std::unique_ptr<AppShimHostBootstrap> bootstrap,
      mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
      mojo::PendingRemote<mojom::AppShim> app_shim);

  // Public for cppgc::MakeGarbageCollected.
  AppShimHost(std::unique_ptr<AppShimHostBootstrap> bootstrap,
              mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
              mojo::PendingRemote<mojom::AppShim> app_shim);
  ~AppShimHost() override;

  AppShimHost(const AppShimHost&) = delete;
  AppShimHost& operator=(const AppShimHost&) = delete;

  // The bundle that the process an audit token names runs from; empty when
  // the process is gone.
  static base::FilePath BundlePathForAuditToken(const audit_token_t& token);

  // What creates this shim's NSWindows; null once the shim has disconnected.
  remote_cocoa::ApplicationHost* GetRemoteCocoaApplicationHost() const;
  // The shim's process; null once the shim has disconnected.
  mojom::AppShim* GetAppShim() const;
  // A window was made in the shim's process.
  void DidCreateWindow();

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  const char* GetClassName() const { return "AppShimHost"; }

 private:
  class Connection;

  // From the connection.
  void OnReopen(bool has_visible_windows);
  void OnFilesOpened(const std::vector<base::FilePath>& files);
  void OnUrlsOpened(const std::vector<GURL>& urls);
  // False when a listener prevented the default, which is to quit the shim.
  bool OnQuitRequested();
  void OnDisconnected();

  // JS API.
  int GetPid() const;
  base::FilePath GetBundlePath() const;
  std::string GetBundleId() const;
  bool IsConnected() const;
  bool SatisfiesCodeRequirement(gin_helper::ErrorThrower thrower,
                                const std::string& requirement);
  void SetBadge(const std::string& label);
  void Bounce(gin::Arguments* args);
  void CancelBounce();
  void Quit();
  void SetMenu(gin::Arguments* args);

  std::unique_ptr<Connection, NativePeerBase::Deleter> connection_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_APP_SHIM_HOST_H_
