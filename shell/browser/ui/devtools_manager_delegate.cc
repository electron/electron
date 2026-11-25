// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/ui/devtools_manager_delegate.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/devtools_agent_host_client_channel.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "electron/grit/electron_resources.h"
#include "net/base/net_errors.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/window_list.h"
#include "shell/common/electron_paths.h"
#include "third_party/inspector_protocol/crdtp/chromium/protocol_traits.h"
#include "third_party/inspector_protocol/crdtp/dispatch.h"
#include "third_party/inspector_protocol/crdtp/protocol_core.h"
#include "third_party/inspector_protocol/crdtp/serializable.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"

namespace electron {

namespace {

class TCPServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, int port)
      : address_(address), port_(port) {}

  // disable copy
  TCPServerSocketFactory(const TCPServerSocketFactory&) = delete;
  TCPServerSocketFactory& operator=(const TCPServerSocketFactory&) = delete;

 private:
  // content::ServerSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    auto socket =
        std::make_unique<net::TCPServerSocket>(nullptr, net::NetLogSource());
    if (socket->ListenWithAddressAndPort(address_, port_, 10) != net::OK)
      return {};

    return socket;
  }
  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* name) override {
    return {};
  }

  std::string address_;
  uint16_t port_;
};

std::unique_ptr<content::DevToolsSocketFactory> CreateSocketFactory() {
  auto& command_line = *base::CommandLine::ForCurrentProcess();
  // See if the user specified a port on the command line (useful for
  // automation). If not, use an ephemeral port by specifying 0.
  int port = 0;
  if (command_line.HasSwitch(switches::kRemoteDebuggingPort)) {
    int temp_port;
    std::string port_str =
        command_line.GetSwitchValueASCII(switches::kRemoteDebuggingPort);
    if (base::StringToInt(port_str, &temp_port) && temp_port >= 0 &&
        temp_port < 65535) {
      port = temp_port;
    } else {
      DLOG(WARNING) << "Invalid http debugger port number " << temp_port;
    }
  }
  return std::make_unique<TCPServerSocketFactory>("127.0.0.1", port);
}

const char kBrowserCloseMethod[] = "Browser.close";

// The window commands below mirror chrome/browser/devtools/protocol/
// browser_handler.cc. The window id is the BaseWindow id, so it matches
// BrowserWindow.id.

NativeWindow* GetWindowForId(int window_id) {
  for (NativeWindow* window : WindowList::GetWindows()) {
    if (window->base_window_id() == window_id && !window->IsClosed())
      return window;
  }
  return nullptr;
}

base::DictValue GetWindowBoundsDict(const NativeWindow* window) {
  std::string_view window_state = "normal";
  if (window->IsMinimized())
    window_state = "minimized";
  if (window->IsMaximized())
    window_state = "maximized";
  if (window->IsFullscreen())
    window_state = "fullscreen";

  // Like Chrome, report the restored bounds of a minimized window.
  const gfx::Rect bounds =
      window->IsMinimized() ? window->GetNormalBounds() : window->GetBounds();
  return base::DictValue()
      .Set("left", bounds.x())
      .Set("top", bounds.y())
      .Set("width", bounds.width())
      .Set("height", bounds.height())
      .Set("windowState", window_state);
}

// Returns the window that shows |web_contents|.
NativeWindow* GetHostWindow(content::WebContents* web_contents) {
  // A <webview> is shown wherever its embedder is.
  web_contents = web_contents->GetOutermostWebContents();
  // A built-in DevTools frontend has no window of its own. Docked, it is part
  // of the inspected contents' view; undocked, it is shown in a widget of its
  // own that is not a NativeWindow. (In Chrome, undocked DevTools is a tab of
  // a TYPE_DEVTOOLS Browser and so does have a window.) A frontend set with
  // setDevToolsWebContents() is the app's own contents and resolves like any
  // other.
  if (InspectableWebContents::FromDevToolsWebContents(web_contents))
    return nullptr;
  // Set for a window's own contents, its WebContentsViews and <webview>s, and
  // kept up to date when a WebContentsView moves to another window.
  auto* relay = NativeWindowRelay::FromWebContents(web_contents);
  NativeWindow* window = relay ? relay->GetNativeWindow() : nullptr;
  if (!window)
    return nullptr;
  // The relay still names the window after a WebContentsView has been removed
  // from it, so check that the contents' view is still inside the window's
  // widget. This checks the views tree rather than the contents' native view,
  // which with offscreen rendering is never inside the widget on macOS and is
  // the owner window's own native view elsewhere.
  auto* api_web_contents = api::WebContents::From(web_contents);
  InspectableWebContents* inspectable_web_contents =
      api_web_contents ? api_web_contents->inspectable_web_contents() : nullptr;
  InspectableWebContentsView* view =
      inspectable_web_contents ? inspectable_web_contents->GetView() : nullptr;
  if (!view || view->GetWidget() != window->widget())
    return nullptr;
  return window;
}

crdtp::DispatchResponse GetWindowForTarget(
    content::DevToolsAgentHostClientChannel* channel,
    const base::DictValue& params,
    base::DictValue* result) {
  std::string target_id;
  if (const base::Value* value = params.Find("targetId")) {
    if (!value->is_string()) {
      return crdtp::DispatchResponse::InvalidParams(
          "'targetId' must be a string");
    }
    target_id = value->GetString();
  } else {
    target_id = channel->GetAgentHost()->GetId();
  }

  scoped_refptr<content::DevToolsAgentHost> host =
      content::DevToolsAgentHost::GetForId(target_id);
  if (!host)
    return crdtp::DispatchResponse::ServerError("No target with given id");
  content::WebContents* web_contents = host->GetWebContents();
  if (!web_contents) {
    return crdtp::DispatchResponse::ServerError(
        "No web contents in the target");
  }

  NativeWindow* window = GetHostWindow(web_contents);
  if (!window || window->IsClosed())
    return crdtp::DispatchResponse::ServerError("Browser window not found");

  result->Set("windowId", window->base_window_id());
  result->Set("bounds", GetWindowBoundsDict(window));
  return crdtp::DispatchResponse::Success();
}

crdtp::DispatchResponse GetWindowBounds(
    content::DevToolsAgentHostClientChannel* channel,
    const base::DictValue& params,
    base::DictValue* result) {
  std::optional<int> window_id = params.FindInt("windowId");
  if (!window_id) {
    return crdtp::DispatchResponse::InvalidParams(
        "'windowId' must be an integer");
  }
  NativeWindow* window = GetWindowForId(*window_id);
  if (!window)
    return crdtp::DispatchResponse::ServerError("Browser window not found");

  result->Set("bounds", GetWindowBoundsDict(window));
  return crdtp::DispatchResponse::Success();
}

crdtp::DispatchResponse SetWindowBounds(
    content::DevToolsAgentHostClientChannel* channel,
    const base::DictValue& params,
    base::DictValue* result) {
  std::optional<int> window_id = params.FindInt("windowId");
  if (!window_id) {
    return crdtp::DispatchResponse::InvalidParams(
        "'windowId' must be an integer");
  }
  const base::DictValue* window_bounds = params.FindDict("bounds");
  if (!window_bounds)
    return crdtp::DispatchResponse::InvalidParams("'bounds' must be an object");

  std::optional<int> left;
  std::optional<int> top;
  std::optional<int> width;
  std::optional<int> height;
  for (auto [name, field] :
       {std::pair{"left", &left}, std::pair{"top", &top},
        std::pair{"width", &width}, std::pair{"height", &height}}) {
    if (const base::Value* value = window_bounds->Find(name)) {
      if (!value->is_int()) {
        return crdtp::DispatchResponse::InvalidParams(
            base::StrCat({"'bounds.", name, "' must be an integer"}));
      }
      *field = value->GetInt();
    }
  }
  std::string window_state = "normal";
  if (const base::Value* value = window_bounds->Find("windowState")) {
    if (!value->is_string()) {
      return crdtp::DispatchResponse::InvalidParams(
          "'bounds.windowState' must be a string");
    }
    window_state = value->GetString();
  }
  if (window_state != "normal" && window_state != "minimized" &&
      window_state != "maximized" && window_state != "fullscreen") {
    return crdtp::DispatchResponse::InvalidParams("Invalid window state: " +
                                                  window_state);
  }

  NativeWindow* window = GetWindowForId(*window_id);
  if (!window)
    return crdtp::DispatchResponse::ServerError("Browser window not found");

  const bool set_bounds = left || top || width || height;
  if (set_bounds && window_state != "normal") {
    return crdtp::DispatchResponse::InvalidParams(
        "The 'minimized', 'maximized' and 'fullscreen' states cannot be "
        "combined with 'left', 'top', 'width' or 'height'");
  }

  if (window_state == "fullscreen") {
    if (!window->IsFullscreen() && !window->IsFullScreenable()) {
      return crdtp::DispatchResponse::ServerError(
          "Window is not fullscreenable");
    }
    if (window->IsMinimized()) {
      return crdtp::DispatchResponse::ServerError(
          "To make minimized window fullscreen, "
          "restore it to normal state first.");
    }
    window->SetFullScreen(true);
  } else if (window_state == "maximized") {
    if (window->IsMinimized() || window->IsFullscreen()) {
      return crdtp::DispatchResponse::ServerError(
          "To maximize a minimized or fullscreen "
          "window, restore it to normal state first.");
    }
    window->Maximize();
  } else if (window_state == "minimized") {
    if (window->IsFullscreen()) {
      return crdtp::DispatchResponse::ServerError(
          "To minimize a fullscreen window, restore it to normal "
          "state first.");
    }
    window->Minimize();
  } else if (window->IsFullscreen()) {
    window->SetFullScreen(false);
  } else if (window->IsMinimized()) {
    window->Restore();
  } else if (window->IsMaximized()) {
    // Restore() only deminiaturizes on macOS.
    window->Unmaximize();
  } else if (set_bounds) {
    gfx::Rect bounds = window->GetBounds();
    bounds.set_x(left.value_or(bounds.x()));
    bounds.set_y(top.value_or(bounds.y()));
    bounds.set_width(width.value_or(bounds.width()));
    bounds.set_height(height.value_or(bounds.height()));
    window->SetBounds(bounds, /*animate=*/false);
  }

  return crdtp::DispatchResponse::Success();
}

using CommandHandler = crdtp::DispatchResponse (*)(
    content::DevToolsAgentHostClientChannel* channel,
    const base::DictValue& params,
    base::DictValue* result);

CommandHandler GetWindowCommandHandler(crdtp::span<uint8_t> method) {
  static constexpr std::pair<const char*, CommandHandler> kHandlers[] = {
      {"Browser.getWindowForTarget", &GetWindowForTarget},
      {"Browser.getWindowBounds", &GetWindowBounds},
      {"Browser.setWindowBounds", &SetWindowBounds},
  };
  for (const auto& [name, handler] : kHandlers) {
    if (crdtp::SpanEquals(crdtp::SpanFrom(name), method))
      return handler;
  }
  return nullptr;
}

// Reads the params of a command, which may be absent.
bool ParseParams(const crdtp::Dispatchable& dispatchable,
                 base::DictValue* params) {
  if (dispatchable.Params().empty())
    return true;
  crdtp::DeserializerState state(std::vector<uint8_t>(
      dispatchable.Params().begin(), dispatchable.Params().end()));
  return crdtp::ProtocolTypeTraits<base::DictValue>::Deserialize(&state,
                                                                 params);
}

void SendResponse(content::DevToolsAgentHostClientChannel* channel,
                  int call_id,
                  crdtp::DispatchResponse response,
                  const base::DictValue& result) {
  std::unique_ptr<crdtp::Serializable> message;
  if (response.IsSuccess()) {
    std::vector<uint8_t> result_bytes;
    crdtp::ProtocolTypeTraits<base::DictValue>::Serialize(result,
                                                          &result_bytes);
    message = crdtp::CreateResponse(
        call_id, crdtp::Serializable::From(std::move(result_bytes)));
  } else {
    message = crdtp::CreateErrorResponse(call_id, std::move(response));
  }
  channel->DispatchProtocolMessageToClient(message->Serialize());
}

}  // namespace

// DevToolsManagerDelegate ---------------------------------------------------

// static
void DevToolsManagerDelegate::StartHttpHandler() {
  base::FilePath session_data;
  base::PathService::Get(DIR_SESSION_DATA, &session_data);
  content::DevToolsAgentHost::StartRemoteDebuggingServer(
      CreateSocketFactory(), session_data, base::FilePath());
}

DevToolsManagerDelegate::DevToolsManagerDelegate() = default;

DevToolsManagerDelegate::~DevToolsManagerDelegate() = default;

void DevToolsManagerDelegate::Inspect(content::DevToolsAgentHost* agent_host) {}

void DevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHostClientChannel* channel,
    base::span<const uint8_t> message,
    NotHandledCallback callback) {
  crdtp::Dispatchable dispatchable(crdtp::SpanFrom(message), std::string_view(),
                                   crdtp::FallthroughCallback());
  DCHECK(dispatchable.ok());
  if (crdtp::SpanEquals(crdtp::SpanFrom(kBrowserCloseMethod),
                        dispatchable.Method())) {
    SendResponse(channel, dispatchable.CallId(),
                 crdtp::DispatchResponse::Success(), base::DictValue());
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce([]() { Browser::Get()->Quit(); }));
    return;
  }
  // Kept for parity with Chrome, which only exposes window management to
  // trusted clients. IsTrusted() defaults to true and no Electron client
  // overrides it, so today any debugger client, webContents.debugger
  // included, can move or resize any window.
  if (channel->GetClient()->IsTrusted()) {
    if (CommandHandler handler =
            GetWindowCommandHandler(dispatchable.Method())) {
      base::DictValue params;
      base::DictValue result;
      // Like Chrome, act first and then reply. Acting can emit window events
      // synchronously (WM_SIZE on Windows, the zoom on macOS, a resize on
      // X11), and a listener that detaches the client deletes |channel|.
      const size_t index = dispatching_channels_.size();
      dispatching_channels_.push_back(channel);
      crdtp::DispatchResponse response =
          ParseParams(dispatchable, &params)
              ? handler(channel, params, &result)
              : crdtp::DispatchResponse::InvalidParams("Invalid parameters");
      DCHECK_EQ(dispatching_channels_.size(), index + 1);
      const bool detached = !dispatching_channels_.back();
      dispatching_channels_.pop_back();
      if (!detached) {
        SendResponse(channel, dispatchable.CallId(), std::move(response),
                     result);
      }
      return;
    }
  }
  std::move(callback).Run(message);
}

void DevToolsManagerDelegate::ClientDetached(
    content::DevToolsAgentHostClientChannel* channel) {
  for (auto& dispatching_channel : dispatching_channels_) {
    if (dispatching_channel == channel)
      dispatching_channel = nullptr;
  }
}

scoped_refptr<content::DevToolsAgentHost>
DevToolsManagerDelegate::CreateNewTarget(const GURL& url,
                                         TargetType target_type,
                                         bool new_window) {
  return nullptr;
}

std::string DevToolsManagerDelegate::GetDiscoveryPageHTML() {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
      IDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE);
}

bool DevToolsManagerDelegate::HasBundledFrontendResources() {
  return true;
}

bool DevToolsManagerDelegate::ShouldUseBundledFrontendResources() {
  return true;
}

content::BrowserContext* DevToolsManagerDelegate::GetDefaultBrowserContext() {
  return ElectronBrowserContext::GetDefaultBrowserContext();
}

}  // namespace electron
