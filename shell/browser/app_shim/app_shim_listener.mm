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
#include "base/apple/scoped_mach_port.h"
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
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/app_shim_constants.h"
#include "shell/common/gin_converters/file_path_converter.h"

namespace electron {

namespace {

// How long a process that connected may take to say hello.
constexpr base::TimeDelta kHelloTimeout = base::Seconds(30);

// The longest name the bootstrap server takes (BOOTSTRAP_MAX_NAME_LEN).
constexpr size_t kMaxServerNameLength = 127;

}  // namespace

// AppShimHostBootstrap ------------------------------------------------------

// static
std::unique_ptr<AppShimHostBootstrap> AppShimHostBootstrap::CreateForChannel(
    mojo::PlatformChannelEndpoint endpoint,
    audit_token_t audit_token,
    Client* client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!endpoint.platform_handle().is_mach_send())
    return nullptr;
  auto bootstrap =
      base::WrapUnique(new AppShimHostBootstrap(audit_token, client));
  mojo::ScopedMessagePipeHandle message_pipe =
      bootstrap->connection_.Connect(std::move(endpoint));
  bootstrap->receiver_.Bind(mojo::PendingReceiver<mojom::AppShimHostBootstrap>(
      std::move(message_pipe)));
  bootstrap->receiver_.set_disconnect_handler(base::BindOnce(
      &AppShimHostBootstrap::Abandon, base::Unretained(bootstrap.get())));
  bootstrap->hello_timer_.Start(FROM_HERE, kHelloTimeout, bootstrap.get(),
                                &AppShimHostBootstrap::Abandon);
  return bootstrap;
}

AppShimHostBootstrap::AppShimHostBootstrap(audit_token_t audit_token,
                                           Client* client)
    : audit_token_(audit_token), client_(client) {}

AppShimHostBootstrap::~AppShimHostBootstrap() = default;

void AppShimHostBootstrap::Abandon() {
  // After the hello the shim closing its bootstrap pipe is expected.
  if (Client* client = std::exchange(client_, nullptr))
    client->OnBootstrapAbandoned(this);  // Deletes this.
}

void AppShimHostBootstrap::OnShimConnected(
    mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
    OnShimConnectedCallback callback) {
  hello_timer_.Stop();
  // One hello per channel.
  Client* client = std::exchange(client_, nullptr);
  if (!client) {
    std::move(callback).Run(mojo::NullReceiver());
    return;
  }
  mojo::PendingRemote<mojom::AppShim> app_shim;
  mojo::PendingReceiver<mojom::AppShim> app_shim_receiver =
      app_shim.InitWithNewPipeAndPassReceiver();
  // A client that accepts takes ownership of this.
  if (client->OnShimConnected(this, std::move(host_receiver),
                              std::move(app_shim))) {
    std::move(callback).Run(std::move(app_shim_receiver));
    return;
  }
  std::move(callback).Run(mojo::NullReceiver());
  client->OnBootstrapAbandoned(this);  // Deletes this.
}

// AppShimListener::Acceptor -------------------------------------------------

// Receives the raw Mach messages shims send to the bootstrap server. Messages
// arrive on a dispatch queue; each connection is passed on to the UI thread.
class AppShimListener::Acceptor {
 public:
  using ConnectCallback =
      base::RepeatingCallback<void(mojo::PlatformChannelEndpoint,
                                   audit_token_t)>;
  using BuildMismatchCallback =
      base::RepeatingCallback<void(audit_token_t,
                                   base::apple::ScopedMachSendRight)>;

  Acceptor(mojo::PlatformChannelServerEndpoint endpoint,
           const std::string& server_name,
           ConnectCallback on_connect,
           BuildMismatchCallback on_build_mismatch)
      : endpoint_(std::move(endpoint)),
        on_connect_(std::move(on_connect)),
        on_build_mismatch_(std::move(on_build_mismatch)),
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
    if (request.header.msgh_size != sizeof(mach_msg_base_t))
      return;
    if (request.header.msgh_id == app_shim::kBuildMismatchMsgId) {
      // The reply right is what the shim waits on; it is let go of once the
      // shim has been identified, which needs its process to be there.
      base::apple::ScopedMachSendRight reply(request.header.msgh_remote_port);
      scoped_message.Disarm();
      ui_task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(on_build_mismatch_, request.trailer.msgh_audit,
                         std::move(reply)));
      return;
    }
    if (request.header.msgh_id != app_shim::kBootstrapMsgId)
      return;

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
  const BuildMismatchCallback on_build_mismatch_;
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
  const bool valid = !name.empty() && std::ranges::all_of(name, [](char c) {
    return base::IsAsciiAlphaNumeric(c) || c == '.' || c == '-' || c == '_';
  });
  if (!valid) {
    *error = "The app shim server name may only hold letters, digits, '.', "
             "'-' and '_'";
    return false;
  }

  mojo::NamedPlatformChannel::Options options;
  options.server_name = base::StrCat({base::apple::BaseBundleID(), ".", name});
  if (options.server_name.size() > kMaxServerNameLength) {
    *error = base::StrCat(
        {"The app shim server name \"", options.server_name, "\" is too long"});
    return false;
  }
  // The marker goes up first, so that a shim that finds the server finds the
  // marker too. See app_shim::BuildMarkerServerName().
  mojo::NamedPlatformChannel::Options marker_options;
  marker_options.server_name =
      app_shim::BuildMarkerServerName(options.server_name);
  if (marker_options.server_name.size() > kMaxServerNameLength) {
    *error = base::StrCat(
        {"The app shim server name \"", options.server_name, "\" is too long"});
    return false;
  }
  mojo::PlatformChannelServerEndpoint marker =
      mojo::NamedPlatformChannel(marker_options).TakeServerEndpoint();
  mojo::NamedPlatformChannel channel(options);
  mojo::PlatformChannelServerEndpoint endpoint = channel.TakeServerEndpoint();
  if (!marker.is_valid() || !endpoint.is_valid()) {
    *error = base::StrCat({"Could not publish the app shim server \"",
                           options.server_name,
                           "\"; another process may already hold that name"});
    return false;
  }

  build_marker_ = std::move(marker);
  acceptor_ = std::make_unique<Acceptor>(
      std::move(endpoint), options.server_name,
      base::BindRepeating(&AppShimListener::OnClientConnected,
                          weak_factory_.GetWeakPtr()),
      base::BindRepeating(&AppShimListener::OnBuildMismatch,
                          weak_factory_.GetWeakPtr()));
  return true;
}

void AppShimListener::OnBuildMismatch(audit_token_t audit_token,
                                      base::apple::ScopedMachSendRight reply) {
  if (!is_listening() || Browser::Get()->is_shutting_down())
    return;
  const base::FilePath bundle_path =
      api::AppShimHost::BundlePathForAuditToken(audit_token);
  // The shim may quit now, and need not wait for what listeners do.
  reply.reset();

  v8::Isolate* const isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  api::App::Get()->Emit("app-shim-version-mismatch", bundle_path);
}

void AppShimListener::OnClientConnected(mojo::PlatformChannelEndpoint endpoint,
                                        audit_token_t audit_token) {
  if (!is_listening() || Browser::Get()->is_shutting_down())
    return;
  if (auto bootstrap = AppShimHostBootstrap::CreateForChannel(
          std::move(endpoint), audit_token, this)) {
    pending_bootstraps_.insert(std::move(bootstrap));
  }
}

void AppShimListener::Stop() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  acceptor_.reset();
  build_marker_.reset();
  pending_bootstraps_.clear();
}

bool AppShimListener::OnShimConnected(
    AppShimHostBootstrap* bootstrap,
    mojo::PendingReceiver<mojom::AppShimHost> host_receiver,
    mojo::PendingRemote<mojom::AppShim> app_shim) {
  auto it = pending_bootstraps_.find(bootstrap);
  if (it == pending_bootstraps_.end() || !is_listening() ||
      Browser::Get()->is_shutting_down()) {
    return false;
  }
  std::unique_ptr<AppShimHostBootstrap> owned =
      std::move(pending_bootstraps_.extract(it).value());

  v8::Isolate* const isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  api::AppShimHost* shim_host = api::AppShimHost::Create(
      isolate, std::move(owned), std::move(host_receiver), std::move(app_shim));
  v8::Local<v8::Object> wrapper;
  if (shim_host->GetWrapper(isolate).ToLocal(&wrapper))
    api::App::Get()->Emit("app-shim-connected", wrapper);
  return true;
}

void AppShimListener::OnBootstrapAbandoned(AppShimHostBootstrap* bootstrap) {
  auto it = pending_bootstraps_.find(bootstrap);
  if (it != pending_bootstraps_.end())
    pending_bootstraps_.erase(it);
}

}  // namespace electron
