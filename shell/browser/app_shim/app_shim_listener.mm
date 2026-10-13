// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/app_shim/app_shim_listener.h"

#include <mach/message.h>

#include <algorithm>
#include <utility>

#include "base/apple/dispatch_source.h"
#include "base/apple/foundation_util.h"
#include "base/apple/mach_logging.h"
#include "base/functional/bind.h"
#include "base/mac/scoped_mach_msg_destroy.h"
#include "base/memory/ptr_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/platform/named_platform_channel.h"
#include "mojo/public/cpp/platform/platform_channel_server_endpoint.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/api/electron_api_app_shim_host.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/app_shim_constants.h"

namespace electron {

// AppShimHostBootstrap ------------------------------------------------------

// static
void AppShimHostBootstrap::CreateForChannel(
    mojo::PlatformChannelEndpoint endpoint,
    audit_token_t audit_token,
    ConnectedCallback on_connected) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!endpoint.platform_handle().is_mach_send())
    return;
  // Owns itself until OnShimConnected or a channel error.
  auto* bootstrap =
      new AppShimHostBootstrap(audit_token, std::move(on_connected));
  mojo::ScopedMessagePipeHandle message_pipe =
      bootstrap->connection_.Connect(std::move(endpoint));
  bootstrap->receiver_.Bind(mojo::PendingReceiver<mojom::AppShimHostBootstrap>(
      std::move(message_pipe)));
  bootstrap->receiver_.set_disconnect_handler(base::BindOnce(
      &AppShimHostBootstrap::OnChannelError, base::Unretained(bootstrap)));
}

AppShimHostBootstrap::AppShimHostBootstrap(audit_token_t audit_token,
                                           ConnectedCallback on_connected)
    : audit_token_(audit_token), on_connected_(std::move(on_connected)) {}

AppShimHostBootstrap::~AppShimHostBootstrap() = default;

void AppShimHostBootstrap::OnChannelError() {
  // Once the shim has said hello, this is owned by what serves it, and the
  // shim closing its bootstrap pipe is expected.
  if (on_connected_)
    delete this;
}

void AppShimHostBootstrap::OnShimConnected(
    mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
    OnShimConnectedCallback callback) {
  if (!on_connected_) {
    // One hello per channel.
    std::move(callback).Run(mojo::NullReceiver());
    return;
  }
  mojo::PendingRemote<mojom::AppShim> app_shim;
  std::move(callback).Run(app_shim.InitWithNewPipeAndPassReceiver());
  std::move(on_connected_)
      .Run(base::WrapUnique(this), std::move(host_receiver),
           std::move(app_shim));
}

// AppShimListener::Acceptor -------------------------------------------------

// Receives the raw Mach messages shims send to the bootstrap server. Messages
// arrive on a dispatch queue; each connection is passed on to the UI thread.
class AppShimListener::Acceptor {
 public:
  using ConnectCallback =
      base::RepeatingCallback<void(mojo::PlatformChannelEndpoint,
                                   audit_token_t)>;

  Acceptor(mojo::PlatformChannelServerEndpoint endpoint,
           const std::string& server_name,
           ConnectCallback on_connect)
      : endpoint_(std::move(endpoint)),
        on_connect_(std::move(on_connect)),
        ui_task_runner_(content::GetUIThreadTaskRunner({})) {
    dispatch_source_ = std::make_unique<base::apple::DispatchSource>(
        server_name.c_str(), port(), ^{
          HandleRequest();
        });
    dispatch_source_->Resume();
  }

  ~Acceptor() {
    // Waits for a running HandleRequest.
    dispatch_source_.reset();
  }

 private:
  mach_port_t port() const {
    return endpoint_.platform_handle().GetMachReceiveRight().get();
  }

  void HandleRequest() {
    struct : mach_msg_base_t {
      mach_msg_audit_trailer_t trailer;
    } request{};
    request.header.msgh_size = sizeof(request);
    request.header.msgh_local_port = port();
    kern_return_t kr = mach_msg(
        &request.header,
        MACH_RCV_MSG | MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0) |
            MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AUDIT),
        0, sizeof(request), port(), MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (kr != KERN_SUCCESS) {
      MACH_LOG(ERROR, kr) << "mach_msg";
      return;
    }

    base::ScopedMachMsgDestroy scoped_message(&request.header);
    if (request.header.msgh_id != app_shim::kBootstrapMsgId ||
        request.header.msgh_size != sizeof(mach_msg_base_t)) {
      return;
    }

    mojo::PlatformChannelEndpoint remote_endpoint(mojo::PlatformHandle(
        base::apple::ScopedMachSendRight(request.header.msgh_remote_port)));
    if (!remote_endpoint.is_valid())
      return;
    scoped_message.Disarm();

    ui_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(on_connect_, std::move(remote_endpoint),
                                  request.trailer.msgh_audit));
  }

  mojo::PlatformChannelServerEndpoint endpoint_;
  const ConnectCallback on_connect_;
  const scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;
  std::unique_ptr<base::apple::DispatchSource> dispatch_source_;
};

// AppShimListener -----------------------------------------------------------

// static
AppShimListener* AppShimListener::Get() {
  static base::NoDestructor<AppShimListener> listener;
  return listener.get();
}

AppShimListener::AppShimListener() = default;
AppShimListener::~AppShimListener() = default;

bool AppShimListener::Start(const std::string& name, std::string* error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (acceptor_) {
    *error = "The app shim server is already started";
    return false;
  }
  const bool valid =
      !name.empty() && name.size() <= 64 &&
      std::ranges::all_of(name, [](char c) {
        return base::IsAsciiAlphaNumeric(c) || c == '.' || c == '-' || c == '_';
      });
  if (!valid) {
    *error = "The app shim server name may only hold letters, digits, '.', "
             "'-' and '_', and at most 64 of them";
    return false;
  }

  mojo::NamedPlatformChannel::Options options;
  options.server_name = base::StrCat({base::apple::BaseBundleID(), ".", name});
  mojo::NamedPlatformChannel channel(options);
  mojo::PlatformChannelServerEndpoint endpoint = channel.TakeServerEndpoint();
  if (!endpoint.is_valid()) {
    *error = base::StrCat({"Could not publish the app shim server \"",
                           options.server_name,
                           "\"; another process may already hold that name"});
    return false;
  }

  acceptor_ = std::make_unique<Acceptor>(
      std::move(endpoint), options.server_name,
      base::BindRepeating(
          [](base::WeakPtr<AppShimListener> listener,
             mojo::PlatformChannelEndpoint endpoint,
             audit_token_t audit_token) {
            if (!listener || !listener->is_listening())
              return;
            AppShimHostBootstrap::CreateForChannel(
                std::move(endpoint), audit_token,
                base::BindOnce(&AppShimListener::OnShimConnected, listener));
          },
          weak_factory_.GetWeakPtr()));
  return true;
}

void AppShimListener::Stop() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  acceptor_.reset();
}

void AppShimListener::OnShimConnected(
    std::unique_ptr<AppShimHostBootstrap> bootstrap,
    mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
    mojo::PendingRemote<mojom::AppShim> app_shim) {
  v8::Isolate* const isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  api::AppShimHost* shim_host =
      api::AppShimHost::Create(isolate, std::move(bootstrap),
                               std::move(host_receiver), std::move(app_shim));
  v8::Local<v8::Object> wrapper;
  if (shim_host->GetWrapper(isolate).ToLocal(&wrapper))
    api::App::Get()->Emit("app-shim-connected", wrapper);
}

}  // namespace electron
