// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_zoom_controller.h"

#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "content/public/common/page_zoom.h"
#include "net/base/url_util.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::WebContentsZoomController);

namespace atom {

WebContentsZoomController::WebContentsZoomController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  default_zoom_factor_ = content::kEpsilon;
  temporary_zoom_level_ = content::kEpsilon;
  host_zoom_map_ = content::HostZoomMap::GetForWebContents(web_contents);
  zoom_subscription_ = host_zoom_map_->AddZoomLevelChangedCallback(base::Bind(
      &WebContentsZoomController::OnZoomLevelChanged, base::Unretained(this)));
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

void WebContentsZoomController::SetZoomLevel(double level) {
  if (!web_contents()->GetRenderViewHost()->IsRenderViewLive() ||
      content::ZoomValuesEqual(GetZoomLevel(), level))
    return;

  if (!content::ZoomValuesEqual(GetTemporaryZoomLevel(), content::kEpsilon)) {
    temporary_zoom_level_ = content::kEpsilon;
  }

  auto new_zoom_factor = content::ZoomLevelToZoomFactor(level);
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (entry) {
    std::string host = net::GetHostOrSpecFromURL(entry->GetURL());
    // When new zoom level varies from kZoomFactor, it takes preference.
    if (!content::ZoomValuesEqual(GetDefaultZoomFactor(), new_zoom_factor))
      host_zoom_factor_[host] = new_zoom_factor;
    content::HostZoomMap::SetZoomLevel(web_contents(), level);
    // Notify observers of zoom level changes.
    FOR_EACH_OBSERVER(WebContentsZoomController::Observer, observers_,
                      OnZoomLevelChanged(web_contents(), level, false));
  }
}

double WebContentsZoomController::GetZoomLevel() {
  return content::HostZoomMap::GetZoomLevel(web_contents());
}

void WebContentsZoomController::SetDefaultZoomFactor(double factor) {
  default_zoom_factor_ = factor;
}

double WebContentsZoomController::GetDefaultZoomFactor() {
  return default_zoom_factor_;
}

bool WebContentsZoomController::UsesTemporaryZoomLevel() {
  return !content::ZoomValuesEqual(temporary_zoom_level_, content::kEpsilon);
}

double WebContentsZoomController::GetTemporaryZoomLevel() {
  return temporary_zoom_level_;
}

void WebContentsZoomController::SetTemporaryZoomLevel(double level) {
  int render_process_id = web_contents()->GetRenderProcessHost()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  host_zoom_map_->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                        level);
  temporary_zoom_level_ = level;
  // Notify observers of zoom level changes.
  FOR_EACH_OBSERVER(WebContentsZoomController::Observer, observers_,
                    OnZoomLevelChanged(web_contents(), level, true));
}

void WebContentsZoomController::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || navigation_handle->IsSamePage())
    return;
  int render_process_id = web_contents()->GetRenderProcessHost()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  host_zoom_map_->ClearTemporaryZoomLevel(render_process_id, render_view_id);
}

void WebContentsZoomController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  if (navigation_handle->IsErrorPage()) {
    content::HostZoomMap::SendErrorPageZoomLevelRefresh(web_contents());
    return;
  }

  if (!navigation_handle->IsSamePage())
    SetZoomFactorOnNavigationIfNeeded(navigation_handle->GetURL());
}

void WebContentsZoomController::WebContentsDestroyed() {
  observers_.Clear();
  host_zoom_factor_.clear();
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
  zoom_subscription_ = host_zoom_map_->AddZoomLevelChangedCallback(base::Bind(
      &WebContentsZoomController::OnZoomLevelChanged, base::Unretained(this)));
}

void WebContentsZoomController::SetZoomFactorOnNavigationIfNeeded(
    const GURL& url) {
  if (content::ZoomValuesEqual(GetDefaultZoomFactor(), content::kEpsilon))
    return;

  if (!content::ZoomValuesEqual(GetTemporaryZoomLevel(), content::kEpsilon)) {
    FOR_EACH_OBSERVER(
        WebContentsZoomController::Observer, observers_,
        OnZoomLevelChanged(web_contents(), GetTemporaryZoomLevel(), true));
    temporary_zoom_level_ = content::kEpsilon;
    return;
  }

  // When kZoomFactor is available, it takes precedence over
  // pref store values but if the host has zoom factor set explicitly
  // then it takes precendence.
  // pref store < kZoomFactor < setZoomLevel
  std::string host = net::GetHostOrSpecFromURL(url);
  double zoom_factor = GetDefaultZoomFactor();
  auto it = host_zoom_factor_.find(host);
  if (it != host_zoom_factor_.end())
    zoom_factor = it->second;
  auto level = content::ZoomFactorToZoomLevel(zoom_factor);
  if (content::ZoomValuesEqual(level, GetZoomLevel()))
    return;

  SetZoomLevel(level);
}

void WebContentsZoomController::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  if (change.mode == content::HostZoomMap::ZOOM_CHANGED_FOR_HOST) {
    auto it = host_zoom_factor_.find(change.host);
    if (it == host_zoom_factor_.end())
      return;
    host_zoom_factor_.insert(
        it, std::make_pair(change.host,
                           content::ZoomLevelToZoomFactor(change.zoom_level)));
  }
}

}  // namespace atom
