// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/web_contents_zoom_controller.h"

#include <string>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/page_type.h"
#include "net/base/url_util.h"
#include "shell/browser/web_contents_zoom_observer.h"
#include "third_party/blink/public/common/page/page_zoom.h"

using content::BrowserThread;

namespace electron {

namespace {

const double kPageZoomEpsilon = 0.001;

}  // namespace

WebContentsZoomController::WebContentsZoomController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<WebContentsZoomController>(*web_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  host_zoom_map_ = content::HostZoomMap::GetForWebContents(web_contents);
  zoom_level_ = host_zoom_map_->GetDefaultZoomLevel();
  default_zoom_factor_ = kPageZoomEpsilon;

  zoom_subscription_ = host_zoom_map_->AddZoomLevelChangedCallback(
      base::BindRepeating(&WebContentsZoomController::OnZoomLevelChanged,
                          base::Unretained(this)));

  UpdateState(std::string());
}

WebContentsZoomController::~WebContentsZoomController() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (auto& observer : observers_) {
    observer.OnZoomControllerDestroyed(this);
  }
}

void WebContentsZoomController::AddObserver(WebContentsZoomObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.AddObserver(observer);
}

void WebContentsZoomController::RemoveObserver(
    WebContentsZoomObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.RemoveObserver(observer);
}

void WebContentsZoomController::SetEmbedderZoomController(
    WebContentsZoomController* controller) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  embedder_zoom_controller_ = controller;
}

bool WebContentsZoomController::SetZoomLevel(double level) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();

  // Cannot zoom in disabled mode. Also, don't allow changing zoom level on
  // a crashed tab, an error page or an interstitial page.
  if (zoom_mode_ == ZOOM_MODE_DISABLED ||
      !web_contents()->GetPrimaryMainFrame()->IsRenderFrameLive())
    return false;

  // Do not actually rescale the page in manual mode.
  if (zoom_mode_ == ZOOM_MODE_MANUAL) {
    // If the zoom level hasn't changed, early out to avoid sending an event.
    if (blink::ZoomValuesEqual(zoom_level_, level))
      return true;

    double old_zoom_level = zoom_level_;
    zoom_level_ = level;

    ZoomChangedEventData zoom_change_data(web_contents(), old_zoom_level,
                                          zoom_level_, true /* temporary */,
                                          zoom_mode_);
    for (auto& observer : observers_)
      observer.OnZoomChanged(zoom_change_data);

    return true;
  }

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  DCHECK(zoom_map);
  DCHECK(!event_data_);
  event_data_ = std::make_unique<ZoomChangedEventData>(
      web_contents(), GetZoomLevel(), level, false /* temporary */, zoom_mode_);

  content::GlobalRenderFrameHostId rfh_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();
  if (zoom_mode_ == ZOOM_MODE_ISOLATED ||
      zoom_map->UsesTemporaryZoomLevel(rfh_id)) {
    zoom_map->SetTemporaryZoomLevel(rfh_id, level);
    ZoomChangedEventData zoom_change_data(web_contents(), zoom_level_, level,
                                          true /* temporary */, zoom_mode_);
    for (auto& observer : observers_)
      observer.OnZoomChanged(zoom_change_data);
  } else {
    if (!entry) {
      // If we exit without triggering an update, we should clear event_data_,
      // else we may later trigger a DCHECK(event_data_).
      event_data_.reset();
      return false;
    }
    std::string host =
        net::GetHostOrSpecFromURL(content::HostZoomMap::GetURLFromEntry(entry));
    zoom_map->SetZoomLevelForHost(host, level);
  }

  DCHECK(!event_data_);
  return true;
}

double WebContentsZoomController::GetZoomLevel() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return zoom_mode_ == ZOOM_MODE_MANUAL
             ? zoom_level_
             : content::HostZoomMap::GetZoomLevel(web_contents());
}

void WebContentsZoomController::SetDefaultZoomFactor(double factor) {
  default_zoom_factor_ = factor;
}

void WebContentsZoomController::SetTemporaryZoomLevel(double level) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::GlobalRenderFrameHostId old_rfh_id_ =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();
  host_zoom_map_->SetTemporaryZoomLevel(old_rfh_id_, level);

  // Notify observers of zoom level changes.
  ZoomChangedEventData zoom_change_data(web_contents(), zoom_level_, level,
                                        true /* temporary */, zoom_mode_);
  for (auto& observer : observers_)
    observer.OnZoomChanged(zoom_change_data);
}

bool WebContentsZoomController::UsesTemporaryZoomLevel() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::GlobalRenderFrameHostId rfh_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();
  return host_zoom_map_->UsesTemporaryZoomLevel(rfh_id);
}

void WebContentsZoomController::SetZoomMode(ZoomMode new_mode) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (new_mode == zoom_mode_)
    return;

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  DCHECK(zoom_map);
  content::GlobalRenderFrameHostId rfh_id =
      web_contents()->GetPrimaryMainFrame()->GetGlobalId();
  double original_zoom_level = GetZoomLevel();

  DCHECK(!event_data_);
  event_data_ = std::make_unique<ZoomChangedEventData>(
      web_contents(), original_zoom_level, original_zoom_level,
      false /* temporary */, new_mode);

  switch (new_mode) {
    case ZOOM_MODE_DEFAULT: {
      content::NavigationEntry* entry =
          web_contents()->GetController().GetLastCommittedEntry();

      if (entry) {
        GURL url = content::HostZoomMap::GetURLFromEntry(entry);
        const std::string host = net::GetHostOrSpecFromURL(url);
        const std::string scheme = url.scheme();

        if (zoom_map->HasZoomLevel(scheme, host)) {
          // If there are other tabs with the same origin, then set this tab's
          // zoom level to match theirs. The temporary zoom level will be
          // cleared below, but this call will make sure this tab re-draws at
          // the correct zoom level.
          double origin_zoom_level =
              zoom_map->GetZoomLevelForHostAndScheme(scheme, host);
          event_data_->new_zoom_level = origin_zoom_level;
          zoom_map->SetTemporaryZoomLevel(rfh_id, origin_zoom_level);
        } else {
          // The host will need a level prior to removing the temporary level.
          // We don't want the zoom level to change just because we entered
          // default mode.
          zoom_map->SetZoomLevelForHost(host, original_zoom_level);
        }
      }
      // Remove per-tab zoom data for this tab. No event callback expected.
      zoom_map->ClearTemporaryZoomLevel(rfh_id);
      break;
    }
    case ZOOM_MODE_ISOLATED: {
      // Unless the zoom mode was |ZOOM_MODE_DISABLED| before this call, the
      // page needs an initial isolated zoom back to the same level it was at
      // in the other mode.
      if (zoom_mode_ != ZOOM_MODE_DISABLED) {
        zoom_map->SetTemporaryZoomLevel(rfh_id, original_zoom_level);
      } else {
        // When we don't call any HostZoomMap set functions, we send the event
        // manually.
        for (auto& observer : observers_)
          observer.OnZoomChanged(*event_data_);
        event_data_.reset();
      }
      break;
    }
    case ZOOM_MODE_MANUAL: {
      // Unless the zoom mode was |ZOOM_MODE_DISABLED| before this call, the
      // page needs to be resized to the default zoom. While in manual mode,
      // the zoom level is handled independently.
      if (zoom_mode_ != ZOOM_MODE_DISABLED) {
        zoom_map->SetTemporaryZoomLevel(rfh_id, GetDefaultZoomLevel());
        zoom_level_ = original_zoom_level;
      } else {
        // When we don't call any HostZoomMap set functions, we send the event
        // manually.
        for (auto& observer : observers_)
          observer.OnZoomChanged(*event_data_);
        event_data_.reset();
      }
      break;
    }
    case ZOOM_MODE_DISABLED: {
      // The page needs to be zoomed back to default before disabling the zoom
      double new_zoom_level = GetDefaultZoomLevel();
      event_data_->new_zoom_level = new_zoom_level;
      zoom_map->SetTemporaryZoomLevel(rfh_id, new_zoom_level);
      break;
    }
  }
  // Any event data we've stored should have been consumed by this point.
  DCHECK(!event_data_);

  zoom_mode_ = new_mode;
}

void WebContentsZoomController::ResetZoomModeOnNavigationIfNeeded(
    const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (zoom_mode_ != ZOOM_MODE_ISOLATED && zoom_mode_ != ZOOM_MODE_MANUAL)
    return;

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  zoom_level_ = zoom_map->GetDefaultZoomLevel();
  double old_zoom_level = zoom_map->GetZoomLevel(web_contents());
  double new_zoom_level = zoom_map->GetZoomLevelForHostAndScheme(
      url.scheme(), net::GetHostOrSpecFromURL(url));

  event_data_ = std::make_unique<ZoomChangedEventData>(
      web_contents(), old_zoom_level, new_zoom_level, false, ZOOM_MODE_DEFAULT);

  // The call to ClearTemporaryZoomLevel() doesn't generate any events from
  // HostZoomMap, but the call to UpdateState() at the end of
  // DidFinishNavigation will notify our observers.
  // Note: it's possible the render_process/frame ids have disappeared (e.g.
  // if we navigated to a new origin), but this won't cause a problem in the
  // call below.
  zoom_map->ClearTemporaryZoomLevel(
      web_contents()->GetPrimaryMainFrame()->GetGlobalId());
  zoom_mode_ = ZOOM_MODE_DEFAULT;
}

void WebContentsZoomController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  if (navigation_handle->IsErrorPage())
    content::HostZoomMap::SendErrorPageZoomLevelRefresh(web_contents());

  if (!navigation_handle->IsSameDocument()) {
    ResetZoomModeOnNavigationIfNeeded(navigation_handle->GetURL());
    SetZoomFactorOnNavigationIfNeeded(navigation_handle->GetURL());

    // If the main frame's content has changed, the new page may have a
    // different zoom level from the old one.
    UpdateState(std::string());
  }

  DCHECK(!event_data_);
}

void WebContentsZoomController::WebContentsDestroyed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // At this point we should no longer be sending any zoom events with this
  // WebContents.
  for (auto& observer : observers_) {
    observer.OnZoomControllerDestroyed(this);
  }

  embedder_zoom_controller_ = nullptr;
}

void WebContentsZoomController::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If our associated HostZoomMap changes, update our subscription.
  content::HostZoomMap* new_host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  if (new_host_zoom_map == host_zoom_map_)
    return;

  host_zoom_map_ = new_host_zoom_map;
  zoom_subscription_ = host_zoom_map_->AddZoomLevelChangedCallback(
      base::BindRepeating(&WebContentsZoomController::OnZoomLevelChanged,
                          base::Unretained(this)));
}

void WebContentsZoomController::SetZoomFactorOnNavigationIfNeeded(
    const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (blink::ZoomValuesEqual(default_zoom_factor(), kPageZoomEpsilon))
    return;

  content::GlobalRenderFrameHostId old_rfh_id_ =
      content::GlobalRenderFrameHostId(old_process_id_, old_view_id_);

  if (host_zoom_map_->UsesTemporaryZoomLevel(old_rfh_id_)) {
    host_zoom_map_->ClearTemporaryZoomLevel(old_rfh_id_);
  }

  if (embedder_zoom_controller_ &&
      embedder_zoom_controller_->UsesTemporaryZoomLevel()) {
    double level = embedder_zoom_controller_->GetZoomLevel();
    SetTemporaryZoomLevel(level);
    return;
  }

  // When kZoomFactor is available, it takes precedence over
  // pref store values but if the host has zoom factor set explicitly
  // then it takes precedence.
  // pref store < kZoomFactor < setZoomLevel
  std::string host = net::GetHostOrSpecFromURL(url);
  std::string scheme = url.scheme();
  double zoom_factor = default_zoom_factor();
  double zoom_level = blink::ZoomFactorToZoomLevel(zoom_factor);
  if (host_zoom_map_->HasZoomLevel(scheme, host)) {
    zoom_level = host_zoom_map_->GetZoomLevelForHostAndScheme(scheme, host);
  }
  if (blink::ZoomValuesEqual(zoom_level, GetZoomLevel()))
    return;

  SetZoomLevel(zoom_level);
}

void WebContentsZoomController::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  UpdateState(change.host);
}

void WebContentsZoomController::UpdateState(const std::string& host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If |host| is empty, all observers should be updated.
  if (!host.empty()) {
    // Use the navigation entry's URL instead of the WebContents' so virtual
    // URLs work (e.g. chrome://settings). http://crbug.com/153950
    content::NavigationEntry* entry =
        web_contents()->GetController().GetLastCommittedEntry();
    if (!entry || host != net::GetHostOrSpecFromURL(
                              content::HostZoomMap::GetURLFromEntry(entry))) {
      return;
    }
  }

  if (event_data_) {
    // For state changes initiated within the ZoomController, information about
    // the change should be sent.
    ZoomChangedEventData zoom_change_data = *event_data_;
    event_data_.reset();
    for (auto& observer : observers_)
      observer.OnZoomChanged(zoom_change_data);
  } else {
    double zoom_level = GetZoomLevel();
    ZoomChangedEventData zoom_change_data(web_contents(), zoom_level,
                                          zoom_level, false, zoom_mode_);
    for (auto& observer : observers_)
      observer.OnZoomChanged(zoom_change_data);
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsZoomController);

}  // namespace electron
