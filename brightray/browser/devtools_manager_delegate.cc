// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_manager_delegate.h"

#include <vector>

#include "browser/net/devtools_network_protocol_handler.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "common/content_client.h"
#include "components/devtools_discovery/basic_target_descriptor.h"
#include "components/devtools_discovery/devtools_discovery_manager.h"
#include "components/devtools_http_handler/devtools_http_handler.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_server_socket.h"
#include "net/socket/stream_socket.h"
#include "ui/base/resource/resource_bundle.h"


namespace brightray {

namespace {

// A hack here:
// Copy from grit/shell_resources.h of chromium repository
// since libcontentchromium doesn't expose content_shell resources.
const int kIDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE = 25500;

class TCPServerSocketFactory
    : public devtools_http_handler::DevToolsHttpHandler::ServerSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, int port)
      : address_(address), port_(port) {
  }

 private:
  // content::DevToolsHttpHandler::ServerSocketFactory.
  scoped_ptr<net::ServerSocket> CreateForHttpServer() override {
    scoped_ptr<net::ServerSocket> socket(
        new net::TCPServerSocket(nullptr, net::NetLog::Source()));
    if (socket->ListenWithAddressAndPort(address_, port_, 10) != net::OK)
      return scoped_ptr<net::ServerSocket>();

    return socket;
  }

  std::string address_;
  uint16 port_;

  DISALLOW_COPY_AND_ASSIGN(TCPServerSocketFactory);
};

scoped_ptr<devtools_http_handler::DevToolsHttpHandler::ServerSocketFactory>
CreateSocketFactory() {
  auto& command_line = *base::CommandLine::ForCurrentProcess();
  // See if the user specified a port on the command line (useful for
  // automation). If not, use an ephemeral port by specifying 0.
  int port = 0;
  if (command_line.HasSwitch(switches::kRemoteDebuggingPort)) {
    int temp_port;
    std::string port_str =
        command_line.GetSwitchValueASCII(switches::kRemoteDebuggingPort);
    if (base::StringToInt(port_str, &temp_port) &&
        temp_port > 0 && temp_port < 65535) {
      port = temp_port;
    } else {
      DLOG(WARNING) << "Invalid http debugger port number " << temp_port;
    }
  }
  return scoped_ptr<
      devtools_http_handler::DevToolsHttpHandler::ServerSocketFactory>(
          new TCPServerSocketFactory("127.0.0.1", port));
}


// DevToolsDelegate --------------------------------------------------------

class DevToolsDelegate :
    public devtools_http_handler::DevToolsHttpHandlerDelegate {
 public:
  DevToolsDelegate();
  virtual ~DevToolsDelegate();

  // devtools_http_handler::DevToolsHttpHandlerDelegate.
  std::string GetDiscoveryPageHTML() override;
  std::string GetFrontendResource(const std::string& path) override;
  std::string GetPageThumbnailData(const GURL& url) override;
  content::DevToolsExternalAgentProxyDelegate* HandleWebSocketConnection(
      const std::string& path) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsDelegate);
};

DevToolsDelegate::DevToolsDelegate() {
}

DevToolsDelegate::~DevToolsDelegate() {
}

std::string DevToolsDelegate::GetDiscoveryPageHTML() {
  return ResourceBundle::GetSharedInstance().GetRawDataResource(
      kIDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE).as_string();
}


std::string DevToolsDelegate::GetFrontendResource(
  const std::string& path) {
  return content::DevToolsFrontendHost::GetFrontendResource(path).as_string();
}

std::string DevToolsDelegate::GetPageThumbnailData(const GURL& url) {
  return std::string();
}

content::DevToolsExternalAgentProxyDelegate*
DevToolsDelegate::HandleWebSocketConnection(const std::string& path) {
  return nullptr;
}

}  // namespace

// DevToolsManagerDelegate ---------------------------------------------------

// static
devtools_http_handler::DevToolsHttpHandler*
DevToolsManagerDelegate::CreateHttpHandler() {
  return new devtools_http_handler::DevToolsHttpHandler(
      CreateSocketFactory(),
      std::string(),
      new DevToolsDelegate,
      base::FilePath(),
      base::FilePath(),
      std::string(),
      GetBrightrayUserAgent());
}

DevToolsManagerDelegate::DevToolsManagerDelegate()
    : handler_(new DevToolsNetworkProtocolHandler) {
  // NB(zcbenz): This call does nothing, the only purpose is to make sure the
  // devtools_discovery module is linked into the final executable on Linux.
  // Though it is possible to achieve this by modifying the gyp settings, it
  // would greatly increase gyp file's complexity, so I chose to instead do
  // this hack.
  devtools_discovery::DevToolsDiscoveryManager::GetInstance();
}

DevToolsManagerDelegate::~DevToolsManagerDelegate() {
}

base::DictionaryValue* DevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHost* agent_host,
    base::DictionaryValue* command) {
  return handler_->HandleCommand(agent_host, command);
}

}  // namespace brightray
