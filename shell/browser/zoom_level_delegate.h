// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ZOOM_LEVEL_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_ZOOM_LEVEL_DELEGATE_H_

#include <string>

#include "components/prefs/pref_service.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/zoom_level_delegate.h"

namespace base {
class DictionaryValue;
class FilePath;
}  // namespace base

class PrefRegistrySimple;

namespace electron {

// A class to manage per-partition default and per-host zoom levels.
// It implements an interface between the content/ zoom
// levels in HostZoomMap and preference system. All changes
// to the per-partition default zoom levels flow through this
// class. Any changes to per-host levels are updated when HostZoomMap calls
// OnZoomLevelChanged.
class ZoomLevelDelegate : public content::ZoomLevelDelegate {
 public:
  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  ZoomLevelDelegate(PrefService* pref_service,
                    const base::FilePath& partition_path);
  ~ZoomLevelDelegate() override;

  // disable copy
  ZoomLevelDelegate(const ZoomLevelDelegate&) = delete;
  ZoomLevelDelegate& operator=(const ZoomLevelDelegate&) = delete;

  void SetDefaultZoomLevelPref(double level);
  double GetDefaultZoomLevelPref() const;

  // content::ZoomLevelDelegate:
  void InitHostZoomMap(content::HostZoomMap* host_zoom_map) override;

 private:
  void ExtractPerHostZoomLevels(
      const base::DictionaryValue* host_zoom_dictionary);

  // This is a callback function that receives notifications from HostZoomMap
  // when per-host zoom levels change. It is used to update the per-host
  // zoom levels (if any) managed by this class (for its associated partition).
  void OnZoomLevelChanged(const content::HostZoomMap::ZoomLevelChange& change);

  PrefService* pref_service_;
  content::HostZoomMap* host_zoom_map_ = nullptr;
  base::CallbackListSubscription zoom_subscription_;
  std::string partition_key_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ZOOM_LEVEL_DELEGATE_H_
