// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_zoom_controller.h"

#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "content/public/common/page_zoom.h"
#include "net/base/url_util.h"

namespace atom {

WebContentsZoomController::WebContentsZoomController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  default_zoom_factor_ = content::kEpsilon;
  host_zoom_map_ = content::HostZoomMap::GetForWebContents(web_contents);
}

WebContentsZoomController::~WebContentsZoomController() {}

void WebContentsZoomController::AddObserver(
    WebContentsZoomController::Observer* observer) {
  observers_.AddObserver(observer);
}

void WebContentsZoomController::RemoveObserver(
    WebContentsZoomController::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void WebContentsZoomController::SetEmbedderZoomController(
    WebContentsZoomController* controller) {
  embedder_zoom_controller_ = controller;
}

void WebContentsZoomController::SetZoomLevel(double level) {
  if (!web_contents()->GetRenderViewHost()->IsRenderViewLive() ||
      content::ZoomValuesEqual(GetZoomLevel(), level) ||
      zoom_mode_ == ZOOM_MODE_DISABLED)
    return;

  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();

  if (zoom_mode_ == ZOOM_MODE_MANUAL) {
    zoom_level_ = level;

    for (Observer& observer : observers_)
      observer.OnZoomLevelChanged(web_contents(), level, true);

    return;
  }

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  if (zoom_mode_ == ZOOM_MODE_ISOLATED ||
      zoom_map->UsesTemporaryZoomLevel(render_process_id, render_view_id)) {
    zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id, level);
    // Notify observers of zoom level changes.
    for (Observer& observer : observers_)
      observer.OnZoomLevelChanged(web_contents(), level, true);
  } else {
    content::HostZoomMap::SetZoomLevel(web_contents(), level);

    // Notify observers of zoom level changes.
    for (Observer& observer : observers_)
      observer.OnZoomLevelChanged(web_contents(), level, false);
  }
}

double WebContentsZoomController::GetZoomLevel() {
  return zoom_mode_ == ZOOM_MODE_MANUAL
             ? zoom_level_
             : content::HostZoomMap::GetZoomLevel(web_contents());
}

void WebContentsZoomController::SetDefaultZoomFactor(double factor) {
  default_zoom_factor_ = factor;
}

double WebContentsZoomController::GetDefaultZoomFactor() {
  return default_zoom_factor_;
}

void WebContentsZoomController::SetTemporaryZoomLevel(double level) {
  old_process_id_ = web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  old_view_id_ = web_contents()->GetRenderViewHost()->GetRoutingID();
  host_zoom_map_->SetTemporaryZoomLevel(old_process_id_, old_view_id_, level);
  // Notify observers of zoom level changes.
  for (Observer& observer : observers_)
    observer.OnZoomLevelChanged(web_contents(), level, true);
}

bool WebContentsZoomController::UsesTemporaryZoomLevel() {
  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  return host_zoom_map_->UsesTemporaryZoomLevel(render_process_id,
                                                render_view_id);
}

void WebContentsZoomController::SetZoomMode(ZoomMode new_mode) {
  if (new_mode == zoom_mode_)
    return;

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  double original_zoom_level = GetZoomLevel();

  switch (new_mode) {
    case ZOOM_MODE_DEFAULT: {
      content::NavigationEntry* entry =
          web_contents()->GetController().GetLastCommittedEntry();

      if (entry) {
        GURL url = content::HostZoomMap::GetURLFromEntry(entry);
        std::string host = net::GetHostOrSpecFromURL(url);

        if (zoom_map->HasZoomLevel(url.scheme(), host)) {
          // If there are other tabs with the same origin, then set this tab's
          // zoom level to match theirs. The temporary zoom level will be
          // cleared below, but this call will make sure this tab re-draws at
          // the correct zoom level.
          double origin_zoom_level =
              zoom_map->GetZoomLevelForHostAndScheme(url.scheme(), host);
          zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                          origin_zoom_level);
        } else {
          // The host will need a level prior to removing the temporary level.
          // We don't want the zoom level to change just because we entered
          // default mode.
          zoom_map->SetZoomLevelForHost(host, original_zoom_level);
        }
      }
      // Remove per-tab zoom data for this tab. No event callback expected.
      zoom_map->ClearTemporaryZoomLevel(render_process_id, render_view_id);
      break;
    }
    case ZOOM_MODE_ISOLATED: {
      // Unless the zoom mode was |ZOOM_MODE_DISABLED| before this call, the
      // page needs an initial isolated zoom back to the same level it was at
      // in the other mode.
      if (zoom_mode_ != ZOOM_MODE_DISABLED) {
        zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                        original_zoom_level);
      } else {
        // When we don't call any HostZoomMap set functions, we send the event
        // manually.
        for (Observer& observer : observers_)
          observer.OnZoomLevelChanged(web_contents(), original_zoom_level,
                                      false);
      }
      break;
    }
    case ZOOM_MODE_MANUAL: {
      // Unless the zoom mode was |ZOOM_MODE_DISABLED| before this call, the
      // page needs to be resized to the default zoom. While in manual mode,
      // the zoom level is handled independently.
      if (zoom_mode_ != ZOOM_MODE_DISABLED) {
        zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                        GetDefaultZoomLevel());
        zoom_level_ = original_zoom_level;
      } else {
        // When we don't call any HostZoomMap set functions, we send the event
        // manually.
        for (Observer& observer : observers_)
          observer.OnZoomLevelChanged(web_contents(), original_zoom_level,
                                      false);
      }
      break;
    }
    case ZOOM_MODE_DISABLED: {
      // The page needs to be zoomed back to default before disabling the zoom
      zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                      GetDefaultZoomLevel());
      break;
    }
  }

  zoom_mode_ = new_mode;
}

void WebContentsZoomController::ResetZoomModeOnNavigationIfNeeded(
    const GURL& url) {
  if (zoom_mode_ != ZOOM_MODE_ISOLATED && zoom_mode_ != ZOOM_MODE_MANUAL)
    return;

  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  zoom_level_ = zoom_map->GetDefaultZoomLevel();
  double new_zoom_level = zoom_map->GetZoomLevelForHostAndScheme(
      url.scheme(), net::GetHostOrSpecFromURL(url));
  for (Observer& observer : observers_)
    observer.OnZoomLevelChanged(web_contents(), new_zoom_level, false);
  zoom_map->ClearTemporaryZoomLevel(render_process_id, render_view_id);
  zoom_mode_ = ZOOM_MODE_DEFAULT;
}

void WebContentsZoomController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  if (navigation_handle->IsErrorPage()) {
    content::HostZoomMap::SendErrorPageZoomLevelRefresh(web_contents());
    return;
  }

  ResetZoomModeOnNavigationIfNeeded(navigation_handle->GetURL());
  SetZoomFactorOnNavigationIfNeeded(navigation_handle->GetURL());
}

void WebContentsZoomController::WebContentsDestroyed() {
  for (Observer& observer : observers_)
    observer.OnZoomControllerWebContentsDestroyed();

  observers_.Clear();
  embedder_zoom_controller_ = nullptr;
}

void WebContentsZoomController::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  // If our associated HostZoomMap changes, update our event subscription.
  content::HostZoomMap* new_host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  if (new_host_zoom_map == host_zoom_map_)
    return;

  host_zoom_map_ = new_host_zoom_map;
}

void WebContentsZoomController::SetZoomFactorOnNavigationIfNeeded(
    const GURL& url) {
  if (content::ZoomValuesEqual(GetDefaultZoomFactor(), content::kEpsilon))
    return;

  if (host_zoom_map_->UsesTemporaryZoomLevel(old_process_id_, old_view_id_)) {
    host_zoom_map_->ClearTemporaryZoomLevel(old_process_id_, old_view_id_);
  }

  if (embedder_zoom_controller_ &&
      embedder_zoom_controller_->UsesTemporaryZoomLevel()) {
    double level = embedder_zoom_controller_->GetZoomLevel();
    SetTemporaryZoomLevel(level);
    return;
  }

  // When kZoomFactor is available, it takes precedence over
  // pref store values but if the host has zoom factor set explicitly
  // then it takes precendence.
  // pref store < kZoomFactor < setZoomLevel
  std::string host = net::GetHostOrSpecFromURL(url);
  std::string scheme = url.scheme();
  double zoom_factor = GetDefaultZoomFactor();
  double zoom_level = content::ZoomFactorToZoomLevel(zoom_factor);
  if (host_zoom_map_->HasZoomLevel(scheme, host)) {
    zoom_level = host_zoom_map_->GetZoomLevelForHostAndScheme(scheme, host);
  }
  if (content::ZoomValuesEqual(zoom_level, GetZoomLevel()))
    return;

  SetZoomLevel(zoom_level);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsZoomController)

}  // namespace atom
