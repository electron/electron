// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/extension_renderer_state.h"

#include <stdint.h>

#include "atom/browser/extensions/tab_helper.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/process_type.h"

using content::BrowserThread;
using content::RenderProcessHost;
using content::RenderViewHost;
using content::WebContents;

//
// ExtensionRendererState::RenderViewHostObserver
//

class ExtensionRendererState::RenderViewHostObserver
    : public content::WebContentsObserver {
 public:
  RenderViewHostObserver(RenderViewHost* host, WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        render_view_host_(host) {
  }

  void RenderViewDeleted(content::RenderViewHost* host) override {
    if (host != render_view_host_)
      return;
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(
            &ExtensionRendererState::ClearTabAndWindowId,
            base::Unretained(ExtensionRendererState::GetInstance()),
            host->GetProcess()->GetID(), host->GetRoutingID()));

    delete this;
  }

 private:
  RenderViewHost* render_view_host_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewHostObserver);
};

//
// ExtensionRendererState::TabObserver
//

// This class listens for notifications about changes in renderer state on the
// UI thread, and notifies the ExtensionRendererState on the IO thread. It
// should only ever be accessed on the UI thread.
class ExtensionRendererState::TabObserver
    : public content::NotificationObserver {
 public:
  TabObserver();
  ~TabObserver() override;

 private:
  // content::NotificationObserver interface.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;
};

ExtensionRendererState::TabObserver::TabObserver() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  registrar_.Add(this,
                 content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, chrome::NOTIFICATION_TAB_PARENTED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

ExtensionRendererState::TabObserver::~TabObserver() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void ExtensionRendererState::TabObserver::Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED: {
      WebContents* web_contents = content::Source<WebContents>(source).ptr();

      int32_t tab_id = extensions::TabHelper::IdForTab(web_contents);
      int32_t window_id =
                extensions::TabHelper::IdForWindowContainingTab(web_contents);

      if (tab_id == -1 || window_id == -1)
        return;

      RenderViewHost* host = content::Details<RenderViewHost>(details).ptr();
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(
              &ExtensionRendererState::SetTabAndWindowId,
              base::Unretained(ExtensionRendererState::GetInstance()),
              host->GetProcess()->GetID(), host->GetRoutingID(),
              tab_id,
              window_id));

      // The observer deletes itself.
      new ExtensionRendererState::RenderViewHostObserver(host, web_contents);

      break;
    }
    case chrome::NOTIFICATION_TAB_PARENTED: {
      WebContents* web_contents = content::Source<WebContents>(source).ptr();

      int32_t tab_id = extensions::TabHelper::IdForTab(web_contents);
      int32_t window_id =
                extensions::TabHelper::IdForWindowContainingTab(web_contents);

      if (tab_id == -1 && window_id == -1)
        return;

      RenderViewHost* host = web_contents->GetRenderViewHost();
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(
              &ExtensionRendererState::SetTabAndWindowId,
              base::Unretained(ExtensionRendererState::GetInstance()),
              host->GetProcess()->GetID(), host->GetRoutingID(),
              tab_id,
              window_id));
      break;
    }
  }
}

//
// ExtensionRendererState
//

ExtensionRendererState::ExtensionRendererState() : observer_(NULL) {
}

ExtensionRendererState::~ExtensionRendererState() {
}

// static
ExtensionRendererState* ExtensionRendererState::GetInstance() {
  return base::Singleton<ExtensionRendererState>::get();
}

void ExtensionRendererState::Init() {
  observer_ = new TabObserver;
}

void ExtensionRendererState::Shutdown() {
  delete observer_;
}

void ExtensionRendererState::SetTabAndWindowId(
    int render_process_host_id, int routing_id, int tab_id, int window_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  RenderId render_id(render_process_host_id, routing_id);
  map_[render_id] = TabAndWindowId(tab_id, window_id);
}

void ExtensionRendererState::ClearTabAndWindowId(
    int render_process_host_id, int routing_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  RenderId render_id(render_process_host_id, routing_id);
  map_.erase(render_id);
}

bool ExtensionRendererState::GetTabAndWindowId(
    const  content::ResourceRequestInfo* info, int* tab_id, int* window_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int render_process_id;
  if (info->GetProcessType() == content::PROCESS_TYPE_PLUGIN) {
    render_process_id = info->GetOriginPID();
  } else {
    render_process_id = info->GetChildID();
  }
  int render_view_id = info->GetRouteID();
  RenderId render_id(render_process_id, render_view_id);
  TabAndWindowIdMap::iterator iter = map_.find(render_id);
  if (iter != map_.end()) {
    *tab_id = iter->second.first;
    *window_id = iter->second.second;
    return true;
  }
  return false;
}
