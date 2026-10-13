// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_APP_SHIM_APP_SHIM_LISTENER_H_
#define ELECTRON_SHELL_BROWSER_APP_SHIM_APP_SHIM_LISTENER_H_

#include <bsm/libbsm.h>

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "electron/shell/common/app_shim.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/system/isolated_connection.h"

namespace electron {

// The first interface a shim talks to, on the channel it handed the listener.
// It deletes itself if the channel closes before the shim says hello; after
// that it is owned by whoever serves the shim, because the isolated connection
// it holds carries every other pipe to that shim.
class AppShimHostBootstrap : public mojom::AppShimHostBootstrap {
 public:
  using ConnectedCallback =
      base::OnceCallback<void(std::unique_ptr<AppShimHostBootstrap>,
                              mojo::PendingReceiver<mojom::AppShimHost>,
                              mojo::PendingRemote<mojom::AppShim>)>;

  // `audit_token` identifies the process that sent `endpoint`.
  static void CreateForChannel(mojo::PlatformChannelEndpoint endpoint,
                               audit_token_t audit_token,
                               ConnectedCallback on_connected);

  ~AppShimHostBootstrap() override;

  AppShimHostBootstrap(const AppShimHostBootstrap&) = delete;
  AppShimHostBootstrap& operator=(const AppShimHostBootstrap&) = delete;

  const audit_token_t& audit_token() const { return audit_token_; }

 private:
  AppShimHostBootstrap(audit_token_t audit_token,
                       ConnectedCallback on_connected);

  void OnChannelError();

  // mojom::AppShimHostBootstrap:
  void OnShimConnected(mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
                       OnShimConnectedCallback callback) override;

  const audit_token_t audit_token_;
  ConnectedCallback on_connected_;
  mojo::IsolatedConnection connection_;
  mojo::Receiver<mojom::AppShimHostBootstrap> receiver_{this};
};

// Publishes a Mach bootstrap server that app shims connect to, and hands each
// shim that connects to the `app` module. Lives on the UI thread.
class AppShimListener {
 public:
  static AppShimListener* Get();

  AppShimListener(const AppShimListener&) = delete;
  AppShimListener& operator=(const AppShimListener&) = delete;

  // Starts listening as "<base bundle id>.<name>". False, with `error` set,
  // when the name is malformed or another process holds it.
  bool Start(const std::string& name, std::string* error);
  void Stop();
  bool is_listening() const { return !!acceptor_; }

 private:
  friend class base::NoDestructor<AppShimListener>;
  class Acceptor;

  AppShimListener();
  ~AppShimListener();

  void OnShimConnected(std::unique_ptr<AppShimHostBootstrap> bootstrap,
                       mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
                       mojo::PendingRemote<mojom::AppShim> app_shim);

  std::unique_ptr<Acceptor> acceptor_;
  base::WeakPtrFactory<AppShimListener> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_APP_SHIM_APP_SHIM_LISTENER_H_
