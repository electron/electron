// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_manager_delegate.h"

#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/devtools_target.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "net/socket/tcp_server_socket.h"
#include "ui/base/resource/resource_bundle.h"

using content::DevToolsAgentHost;
using content::RenderViewHost;
using content::WebContents;
using content::BrowserContext;
using content::DevToolsTarget;
using content::DevToolsHttpHandler;

namespace brightray {

namespace {

// A hack here:
// Copy from grit/shell_resources.h of chromium repository
// since libcontentchromium doesn't expose content_shell resources.
const int kIDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE = 25500;

const char kTargetTypePage[] = "page";
const char kTargetTypeServiceWorker[] = "service_worker";
const char kTargetTypeOther[] = "other";

class TCPServerSocketFactory
    : public content::DevToolsHttpHandler::ServerSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, int port, int backlog)
      : content::DevToolsHttpHandler::ServerSocketFactory(
            address, port, backlog) {}

 private:
  // content::DevToolsHttpHandler::ServerSocketFactory.
  scoped_ptr<net::ServerSocket> Create() const override {
    return scoped_ptr<net::ServerSocket>(
        new net::TCPServerSocket(NULL, net::NetLog::Source()));
  }

  DISALLOW_COPY_AND_ASSIGN(TCPServerSocketFactory);
};

scoped_ptr<content::DevToolsHttpHandler::ServerSocketFactory>
CreateSocketFactory() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
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
  return scoped_ptr<content::DevToolsHttpHandler::ServerSocketFactory>(
      new TCPServerSocketFactory("127.0.0.1", port, 1));
}

class Target : public content::DevToolsTarget {
 public:
  explicit Target(scoped_refptr<DevToolsAgentHost> agent_host);

  std::string GetId() const override { return agent_host_->GetId(); }
  std::string GetParentId() const override { return std::string(); }
  std::string GetType() const override {
    switch (agent_host_->GetType()) {
      case DevToolsAgentHost::TYPE_WEB_CONTENTS:
        return kTargetTypePage;
      case DevToolsAgentHost::TYPE_SERVICE_WORKER:
        return kTargetTypeServiceWorker;
      default:
        break;
    }
    return kTargetTypeOther;
  }
  std::string GetTitle() const override { return agent_host_->GetTitle(); }
  std::string GetDescription() const override { return std::string(); }
  GURL GetURL() const override { return agent_host_->GetURL(); }
  GURL GetFaviconURL() const override { return favicon_url_; }
  base::TimeTicks GetLastActivityTime() const override {
    return last_activity_time_;
  }
  bool IsAttached() const override { return agent_host_->IsAttached(); }
  scoped_refptr<DevToolsAgentHost> GetAgentHost() const override {
    return agent_host_;
  }
  bool Activate() const override;
  bool Close() const override;

 private:
  scoped_refptr<DevToolsAgentHost> agent_host_;
  GURL favicon_url_;
  base::TimeTicks last_activity_time_;
};

Target::Target(scoped_refptr<DevToolsAgentHost> agent_host)
    : agent_host_(agent_host) {
  if (WebContents* web_contents = agent_host_->GetWebContents()) {
    content::NavigationController& controller = web_contents->GetController();
    content::NavigationEntry* entry = controller.GetActiveEntry();
    if (entry != NULL && entry->GetURL().is_valid())
      favicon_url_ = entry->GetFavicon().url;
    last_activity_time_ = web_contents->GetLastActiveTime();
  }
}

bool Target::Activate() const {
  return agent_host_->Activate();
}

bool Target::Close() const {
  return agent_host_->Close();
}

// DevToolsDelegate --------------------------------------------------------

class DevToolsDelegate : public content::DevToolsHttpHandlerDelegate {
 public:
  DevToolsDelegate();
  virtual ~DevToolsDelegate();

  // content::DevToolsHttpHandlerDelegate.
  std::string GetDiscoveryPageHTML() override;
  bool BundlesFrontendResources() override;
  base::FilePath GetDebugFrontendDir() override;
  scoped_ptr<net::StreamListenSocket> CreateSocketForTethering(
      net::StreamListenSocket::Delegate* delegate,
      std::string* name) override;

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

bool DevToolsDelegate::BundlesFrontendResources() {
  return true;
}

base::FilePath DevToolsDelegate::GetDebugFrontendDir() {
  return base::FilePath();
}

scoped_ptr<net::StreamListenSocket>
DevToolsDelegate::CreateSocketForTethering(
    net::StreamListenSocket::Delegate* delegate,
    std::string* name) {
  return scoped_ptr<net::StreamListenSocket>();
}

}  // namespace

// DevToolsManagerDelegate ---------------------------------------------------

// static
content::DevToolsHttpHandler*
DevToolsManagerDelegate::CreateHttpHandler() {
  return DevToolsHttpHandler::Start(CreateSocketFactory(),
                                    std::string(),
                                    new DevToolsDelegate,
                                    base::FilePath());
}

DevToolsManagerDelegate::DevToolsManagerDelegate() {
}

DevToolsManagerDelegate::~DevToolsManagerDelegate() {
}

base::DictionaryValue* DevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHost* agent_host,
    base::DictionaryValue* command) {
  return NULL;
}

std::string DevToolsManagerDelegate::GetPageThumbnailData(
    const GURL& url) {
  return std::string();
}

scoped_ptr<content::DevToolsTarget>
DevToolsManagerDelegate::CreateNewTarget(const GURL& url) {
  return scoped_ptr<content::DevToolsTarget>();
}

void DevToolsManagerDelegate::EnumerateTargets(TargetCallback callback) {
  TargetList targets;
  for (const auto& agent_host : DevToolsAgentHost::GetOrCreateAll()) {
    targets.push_back(new Target(agent_host));
  }
  callback.Run(targets);
}

}  // namespace brightray
