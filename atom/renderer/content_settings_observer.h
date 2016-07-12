// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
#define ATOM_RENDERER_CONTENT_SETTINGS_OBSERVER_H_

#include "base/observer_list.h"

namespace base {
class DictionaryValue;
}

namespace atom {

// Interface for classes that listen to changes to content settings.
class ContentSettingsObserver {
 public:
  virtual void OnContentSettingsChanged(
      const base::DictionaryValue& content_settings) = 0;

  virtual ~ContentSettingsObserver() {}
};

typedef base::ObserverList<ContentSettingsObserver> ContentSettingsObserverList;

}  // namespace atom

#endif  // ATOM_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
