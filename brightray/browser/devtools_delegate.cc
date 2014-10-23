// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/devtools_delegate.h"

#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/browser_context.h"
#include "browser/inspectable_web_contents.h"
#include "browser/default_web_contents_delegate.h"
#include "browser/inspectable_web_contents_delegate.h"
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
#include "net/socket/tcp_listen_socket.h"
#include "ui/base/resource/resource_bundle.h"

using content::DevToolsAgentHost;
using content::RenderViewHost;
using content::WebContents;
using content::BrowserContext;
using content::DevToolsTarget;
using content::DevToolsHttpHandler;

namespace {

// A hack here:
// Copy from grit/shell_resources.h of chromium repository
// since libcontentchromium doesn't expose content_shell resources.
const int kIDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE = 25500;

const char kTargetTypePage[] = "page";

net::StreamListenSocketFactory* CreateSocketFactory() {
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
  return new net::TCPListenSocketFactory("127.0.0.1", port);
}

class Target : public content::DevToolsTarget {
 public:
  explicit Target(WebContents* web_contents);

  virtual std::string GetId() const override { return id_; }
  virtual std::string GetParentId() const { return std::string(); }
  virtual std::string GetType() const override { return kTargetTypePage; }
  virtual std::string GetTitle() const override { return title_; }
  virtual std::string GetDescription() const override { return std::string(); }
  virtual GURL GetURL() const override { return url_; }
  virtual GURL GetFaviconURL() const override { return favicon_url_; }
  virtual base::TimeTicks GetLastActivityTime() const override {
    return last_activity_time_;
  }
  virtual bool IsAttached() const override {
    return agent_host_->IsAttached();
  }
  virtual scoped_refptr<DevToolsAgentHost> GetAgentHost() const override {
    return agent_host_;
  }
  virtual bool Activate() const override;
  virtual bool Close() const override;

 private:
  scoped_refptr<DevToolsAgentHost> agent_host_;
  std::string id_;
  std::string title_;
  GURL url_;
  GURL favicon_url_;
  base::TimeTicks last_activity_time_;
};

Target::Target(WebContents* web_contents) {
  agent_host_ = DevToolsAgentHost::GetOrCreateFor(web_contents);
  id_ = agent_host_->GetId();
  title_ = base::UTF16ToUTF8(web_contents->GetTitle());
  url_ = web_contents->GetURL();
  content::NavigationController& controller = web_contents->GetController();
  content::NavigationEntry* entry = controller.GetActiveEntry();
  if (entry != NULL && entry->GetURL().is_valid())
    favicon_url_ = entry->GetFavicon().url;
  last_activity_time_ = web_contents->GetLastActiveTime();
}

bool Target::Activate() const {
  WebContents* web_contents = agent_host_->GetWebContents();
  if (!web_contents)
    return false;
  web_contents->GetDelegate()->ActivateContents(web_contents);
  return true;
}

bool Target::Close() const {
  WebContents* web_contents = agent_host_->GetWebContents();
  if (!web_contents)
    return false;
  web_contents->GetRenderViewHost()->ClosePage();
  return true;
}

}  // namespace

namespace brightray {

DevToolsDelegate::DevToolsDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  std::string frontend_url;
  devtools_http_handler_ = DevToolsHttpHandler::Start(
      CreateSocketFactory(), frontend_url, this, base::FilePath());
}

DevToolsDelegate::~DevToolsDelegate() {
}

void DevToolsDelegate::Stop() {
  // The call below destroys this.
  devtools_http_handler_->Stop();
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

std::string DevToolsDelegate::GetPageThumbnailData(const GURL& url) {
  return std::string();
}

scoped_ptr<DevToolsTarget>
DevToolsDelegate::CreateNewTarget(const GURL& url) {
  content::WebContents::CreateParams create_params(
      new brightray::BrowserContext());
  brightray::InspectableWebContents* web_contents =
      brightray::InspectableWebContents::Create(create_params);
  web_contents->SetDelegate(new brightray::InspectableWebContentsDelegate());
  return scoped_ptr<DevToolsTarget>(new Target(web_contents->GetWebContents()));
}

void DevToolsDelegate::EnumerateTargets(TargetCallback callback) {
  TargetList targets;
  std::vector<WebContents*> wc_list =
      content::DevToolsAgentHost::GetInspectableWebContents();
  for (std::vector<WebContents*>::iterator it = wc_list.begin();
       it != wc_list.end();
       ++it) {
    targets.push_back(new Target(*it));
  }
  callback.Run(targets);
}

scoped_ptr<net::StreamListenSocket>
DevToolsDelegate::CreateSocketForTethering(
    net::StreamListenSocket::Delegate* delegate,
    std::string* name) {
  return scoped_ptr<net::StreamListenSocket>();
}

}  // namespace brightray
