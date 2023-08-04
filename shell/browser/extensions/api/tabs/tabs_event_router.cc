// Copyright (c) 2023 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/tabs/tabs_event_router.h"

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "chrome/browser/browser_process.h"
#include "components/zoom/zoom_controller.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/mojom/event_dispatcher.mojom-forward.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/api/tabs/tabs_window_api.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/common/extensions/api/tabs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/page/page_zoom.h"

using base::Value;
using content::WebContents;
using zoom::ZoomController;

using electron::WebContentsZoomController;

namespace extensions {

// namespace {
// void ZoomModeToZoomSettings(ZoomController::ZoomMode zoom_mode,
//                             api::tabs::ZoomSettings* zoom_settings) {
//   DCHECK(zoom_settings);
//   switch (zoom_mode) {
//     case ZoomController::ZOOM_MODE_DEFAULT:
//       zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
//       zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN;
//       break;
//     case ZoomController::ZOOM_MODE_ISOLATED:
//       zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
//       zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
//       break;
//     case ZoomController::ZOOM_MODE_MANUAL:
//       zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_MANUAL;
//       zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
//       break;
//     case ZoomController::ZOOM_MODE_DISABLED:
//       zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_DISABLED;
//       zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
//       break;
//   }
// }
// }  // namespace

TabsEventRouter::TabsEventRouter(content::BrowserContext* context)
    : context_(context) {}

TabsEventRouter::~TabsEventRouter() = default;

void TabsEventRouter::OnZoomControllerDestroyed(
    WebContentsZoomController* zoom_controller) {
  if (zoom_scoped_observations_.IsObservingSource(zoom_controller)) {
    zoom_scoped_observations_.RemoveObservation(zoom_controller);
  }
}

void TabsEventRouter::OnZoomChanged(
    const electron::WebContentsZoomController::ZoomChangedEventData& data) {
  DCHECK(web_contents);
  auto* api_web_contents = electron::api::WebContents::From(data.web_contents);
  int tab_id = api_web_contents ? api_web_contents->ID() : -1;
  if (tab_id < 0)
    return;

  // Prepare the zoom change information.
  api::tabs::OnZoomChange::ZoomChangeInfo zoom_change_info;
  zoom_change_info.tab_id = tab_id;
  zoom_change_info.old_zoom_factor =
      blink::PageZoomLevelToZoomFactor(data.old_zoom_level);
  zoom_change_info.new_zoom_factor =
      blink::PageZoomLevelToZoomFactor(data.new_zoom_level);
  // ZoomModeToZoomSettings(data.zoom_mode, &zoom_change_info.zoom_settings);

  // Dispatch the |onZoomChange| event.
  DispatchEvent(data.web_contents->GetBrowserContext(),
                events::TABS_ON_ZOOM_CHANGE,
                api::tabs::OnZoomChange::kEventName,
                api::tabs::OnZoomChange::Create(zoom_change_info),
                EventRouter::USER_GESTURE_UNKNOWN);
}

void TabsEventRouter::DispatchEvent(
    content::BrowserContext* context,
    events::HistogramValue histogram_value,
    const std::string& event_name,
    base::Value::List args,
    EventRouter::UserGestureState user_gesture) {
  EventRouter* event_router = EventRouter::Get(context);
  if (!event_router)
    return;

  auto event = std::make_unique<Event>(histogram_value, event_name,
                                       std::move(args), context);
  event->user_gesture = user_gesture;
  event_router->BroadcastEvent(std::move(event));
}

void TabsEventRouter::RegisterForTabNotifications(WebContents* contents) {
  zoom_scoped_observations_.AddObservation(
      WebContentsZoomController::FromWebContents(contents));
}

void TabsEventRouter::UnregisterForTabNotifications(WebContents* contents) {
  if (auto* zoom_controller =
          WebContentsZoomController::FromWebContents(contents);
      zoom_scoped_observations_.IsObservingSource(zoom_controller)) {
    zoom_scoped_observations_.RemoveObservation(zoom_controller);
  }
}

}  // namespace extensions