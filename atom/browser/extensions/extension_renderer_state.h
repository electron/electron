// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_EXTENSION_RENDERER_STATE_H_
#define ATOM_BROWSER_EXTENSIONS_EXTENSION_RENDERER_STATE_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/singleton.h"

namespace content {
class ResourceRequestInfo;
}

// This class keeps track of renderer state for use on the IO thread. All
// methods should be called on the IO thread except for Init and Shutdown.
class ExtensionRendererState {
 public:
  static ExtensionRendererState* GetInstance();

  // These are called on the UI thread to start and stop listening to tab
  // notifications.
  void Init();
  void Shutdown();

  // Looks up the tab and window ID for a given request. Returns true if we have
  // the IDs in our map. Called on the IO thread.
  bool GetTabAndWindowId(
      const content::ResourceRequestInfo* info, int* tab_id, int* window_id);

 private:
  class RenderViewHostObserver;
  class TabObserver;
  friend class TabObserver;
  friend struct base::DefaultSingletonTraits<ExtensionRendererState>;

  typedef std::pair<int, int> RenderId;
  typedef std::pair<int, int> TabAndWindowId;
  typedef std::map<RenderId, TabAndWindowId> TabAndWindowIdMap;

  ExtensionRendererState();
  ~ExtensionRendererState();

  // Adds or removes a render view from our map.
  void SetTabAndWindowId(
      int render_process_host_id, int routing_id, int tab_id, int window_id);
  void ClearTabAndWindowId(
      int render_process_host_id, int routing_id);

  TabObserver* observer_;
  TabAndWindowIdMap map_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRendererState);
};

#endif  // ATOM_BROWSER_EXTENSIONS_EXTENSION_RENDERER_STATE_H_
