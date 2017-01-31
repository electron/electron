// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_WEB_CONTENTS_ZOOM_CONTROLLER_H_
#define ATOM_BROWSER_WEB_CONTENTS_ZOOM_CONTROLLER_H_

#include <map>
#include <string>

#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace atom {

// Manages the zoom changes of WebContents.
class WebContentsZoomController
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsZoomController> {
 public:
  class Observer {
   public:
    virtual void OnZoomLevelChanged(content::WebContents* web_contents,
                                    double level,
                                    bool is_temporary) {}

   protected:
    virtual ~Observer() {}
  };

  explicit WebContentsZoomController(content::WebContents* web_contents);
  ~WebContentsZoomController() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetEmbedderZoomController(WebContentsZoomController* controller);

  // Methods for managing zoom levels.
  void SetZoomLevel(double level);
  double GetZoomLevel();
  void SetDefaultZoomFactor(double factor);
  double GetDefaultZoomFactor();
  void SetTemporaryZoomLevel(double level);
  bool UsesTemporaryZoomLevel();

 protected:
  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void WebContentsDestroyed() override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;

 private:
  friend class content::WebContentsUserData<WebContentsZoomController>;

  // Called after a navigation has committed to set default zoom factor.
  void SetZoomFactorOnNavigationIfNeeded(const GURL& url);

  // Track zoom changes of a host in other instances of a partition.
  void OnZoomLevelChanged(const content::HostZoomMap::ZoomLevelChange& change);

  // kZoomFactor.
  double default_zoom_factor_;
  double temporary_zoom_level_;

  int old_process_id_;
  int old_view_id_;

  WebContentsZoomController* embedder_zoom_controller_;

  // Map between zoom factor and hosts in this webContent.
  std::map<std::string, double> host_zoom_factor_;

  base::ObserverList<Observer> observers_;

  content::HostZoomMap* host_zoom_map_;

  std::unique_ptr<content::HostZoomMap::Subscription> zoom_subscription_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsZoomController);
};

}  // namespace atom

#endif  // ATOM_BROWSER_WEB_CONTENTS_ZOOM_CONTROLLER_H_
