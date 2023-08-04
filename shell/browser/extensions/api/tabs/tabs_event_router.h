// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_EVENT_ROUTER_H_
#define SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_EVENT_ROUTER_H_

#include <map>
#include <set>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/event_router.h"
#include "shell/browser/extensions/api/tabs/tabs_api.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/browser/web_contents_zoom_observer.h"

namespace content {
class WebContents;
}

namespace extensions {

// The TabsEventRouter listens to tab events and routes them to listeners inside
// extension process renderers.
// TabsEventRouter will only route events from windows/tabs within a context to
// extension processes in the same context.
class TabsEventRouter : public electron::WebContentsZoomObserver {
 public:
  explicit TabsEventRouter(content::BrowserContext* context);

  TabsEventRouter(const TabsEventRouter&) = delete;
  TabsEventRouter& operator=(const TabsEventRouter&) = delete;

  ~TabsEventRouter() override;

  // WebContentsZoomController::Observer
  void OnZoomChanged(
      const electron::WebContentsZoomController::ZoomChangedEventData& data)
      override;
  void OnZoomControllerDestroyed(
      electron::WebContentsZoomController* controller) override;

  // Register ourselves to receive the various notifications we are interested
  // in for a tab. Also create tab entry to observe web contents notifications.
  void RegisterForTabNotifications(content::WebContents* contents);

 private:
  // The DispatchEvent methods forward events to the |context|'s event router.
  // The TabsEventRouter listens to events for all contexts,
  // so we avoid duplication by dropping events destined for other contexts.
  void DispatchEvent(content::BrowserContext* context,
                     events::HistogramValue histogram_value,
                     const std::string& event_name,
                     base::Value::List args,
                     EventRouter::UserGestureState user_gesture);

  // Removes notifications and tab entry added in RegisterForTabNotifications.
  void UnregisterForTabNotifications(content::WebContents* contents);

  // The main context that owns this event router.
  raw_ptr<content::BrowserContext> context_;

  base::ScopedMultiSourceObservation<electron::WebContentsZoomController,
                                     electron::WebContentsZoomObserver>
      zoom_scoped_observations_{this};
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_EVENT_ROUTER_H_