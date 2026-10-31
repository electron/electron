// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_APP_SHIM_APP_SHIM_LISTENER_H_
#define ELECTRON_SHELL_BROWSER_APP_SHIM_APP_SHIM_LISTENER_H_

#include <bsm/libbsm.h>

#include <memory>
#include <set>
#include <string>

#include "base/apple/scoped_mach_port.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/timer/timer.h"
#include "electron/shell/common/app_shim.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/platform/platform_channel_endpoint.h"
#include "mojo/public/cpp/platform/platform_channel_server_endpoint.h"
#include "mojo/public/cpp/system/isolated_connection.h"

namespace electron {

// The first interface a shim talks to, on the channel it handed the listener.
// The listener owns it until the shim says hello, and drops it if the shim
// goes away or takes too long first. After the hello it belongs to whoever
// serves the shim.
class AppShimHostBootstrap : public mojom::AppShimHostBootstrap {
 public:
  class Client {
   public:
    // The shim said hello. Returning false turns it away.
    virtual bool OnShimConnected(
        AppShimHostBootstrap* bootstrap,
        mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
        mojo::PendingRemote<mojom::AppShim> app_shim) = 0;
    // The shim went away, or timed out, before it said hello.
    virtual void OnBootstrapAbandoned(AppShimHostBootstrap* bootstrap) = 0;

   protected:
    virtual ~Client() = default;
  };

  // `audit_token` identifies the process that sent `endpoint`. Null when
  // `endpoint` is not usable. `client` must outlive the result.
  static std::unique_ptr<AppShimHostBootstrap> CreateForChannel(
      mojo::PlatformChannelEndpoint endpoint,
      audit_token_t audit_token,
      Client* client);

  ~AppShimHostBootstrap() override;

  AppShimHostBootstrap(const AppShimHostBootstrap&) = delete;
  AppShimHostBootstrap& operator=(const AppShimHostBootstrap&) = delete;

  const audit_token_t& audit_token() const { return audit_token_; }

 private:
  AppShimHostBootstrap(audit_token_t audit_token, Client* client);

  void Abandon();

  // mojom::AppShimHostBootstrap:
  void OnShimConnected(mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
                       OnShimConnectedCallback callback) override;

  const audit_token_t audit_token_;
  // Null once the shim has said hello.
  raw_ptr<Client> client_;
  base::OneShotTimer hello_timer_;
  mojo::IsolatedConnection connection_;
  mojo::Receiver<mojom::AppShimHostBootstrap> receiver_{this};
};

// Publishes a Mach bootstrap server that app shims connect to, and hands each
// shim that connects to the `app` module. Lives on the UI thread.
class AppShimListener : private AppShimHostBootstrap::Client {
 public:
  static AppShimListener* Get();

  AppShimListener(const AppShimListener&) = delete;
  AppShimListener& operator=(const AppShimListener&) = delete;

  // Starts listening as "<base bundle id>.<name>". False, with `error` set,
  // when the name is malformed or another process holds it.
  bool Start(const std::string& name, std::string* error);
  // Stops listening and drops the shims that have not said hello yet.
  void Stop();
  bool is_listening() const { return !!acceptor_; }

 private:
  friend class base::NoDestructor<AppShimListener>;
  class Acceptor;

  AppShimListener();
  ~AppShimListener() override;

  void OnClientConnected(mojo::PlatformChannelEndpoint endpoint,
                         audit_token_t audit_token);
  // A shim that loaded another build of Electron than this one gave up. It
  // waits for `reply` to be let go of before it quits.
  void OnBuildMismatch(audit_token_t audit_token,
                       base::apple::ScopedMachSendRight reply);

  // AppShimHostBootstrap::Client:
  bool OnShimConnected(AppShimHostBootstrap* bootstrap,
                       mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
                       mojo::PendingRemote<mojom::AppShim> app_shim) override;
  void OnBootstrapAbandoned(AppShimHostBootstrap* bootstrap) override;

  // Published for shims to look up and never read from.
  mojo::PlatformChannelServerEndpoint build_marker_;
  std::unique_ptr<Acceptor> acceptor_;
  // Shims that have connected and not said hello yet.
  std::set<std::unique_ptr<AppShimHostBootstrap>, base::UniquePtrComparator>
      pending_bootstraps_;
  base::WeakPtrFactory<AppShimListener> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_APP_SHIM_APP_SHIM_LISTENER_H_
