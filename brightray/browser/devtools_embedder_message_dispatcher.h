// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_DEVTOOLS_EMBEDDER_MESSAGE_DISPATCHER_H_
#define BRIGHTRAY_BROWSER_DEVTOOLS_EMBEDDER_MESSAGE_DISPATCHER_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class ListValue;
class Value;
}

namespace brightray {

/**
 * Dispatcher for messages sent from the DevTools frontend running in an
 * isolated renderer (on chrome-devtools://) to the embedder in the browser.
 *
 * The messages are sent via InspectorFrontendHost.sendMessageToEmbedder method.
 */
class DevToolsEmbedderMessageDispatcher {
 public:
  class Delegate {
   public:
    using DispatchCallback = base::Callback<void(const base::Value*)>;

    virtual ~Delegate() {}

    virtual void ActivateWindow() = 0;
    virtual void CloseWindow() = 0;
    virtual void LoadCompleted() = 0;
    virtual void SetInspectedPageBounds(const gfx::Rect& rect) = 0;
    virtual void InspectElementCompleted() = 0;
    virtual void InspectedURLChanged(const std::string& url) = 0;
    virtual void SetIsDocked(const DispatchCallback& callback,
                             bool is_docked) = 0;
    virtual void OpenInNewTab(const std::string& url) = 0;
    virtual void SaveToFile(const std::string& url,
                            const std::string& content,
                            bool save_as) = 0;
    virtual void AppendToFile(const std::string& url,
                              const std::string& content) = 0;
    virtual void RequestFileSystems() = 0;
    virtual void AddFileSystem(const std::string& file_system_path) = 0;
    virtual void RemoveFileSystem(const std::string& file_system_path) = 0;
    virtual void UpgradeDraggedFileSystemPermissions(
        const std::string& file_system_url) = 0;
    virtual void IndexPath(int index_request_id,
                           const std::string& file_system_path) = 0;
    virtual void StopIndexing(int index_request_id) = 0;
    virtual void LoadNetworkResource(const DispatchCallback& callback,
                                     const std::string& url,
                                     const std::string& headers,
                                     int stream_id) = 0;
    virtual void SearchInPath(int search_request_id,
                              const std::string& file_system_path,
                              const std::string& query) = 0;
    virtual void SetWhitelistedShortcuts(const std::string& message) = 0;
    virtual void ZoomIn() = 0;
    virtual void ZoomOut() = 0;
    virtual void ResetZoom() = 0;
    virtual void SetDevicesUpdatesEnabled(bool enabled) = 0;
    virtual void DispatchProtocolMessageFromDevToolsFrontend(
        const std::string& message) = 0;
    virtual void SendJsonRequest(const DispatchCallback& callback,
                                 const std::string& browser_id,
                                 const std::string& url) = 0;
    virtual void GetPreferences(const DispatchCallback& callback) = 0;
    virtual void SetPreference(const std::string& name,
                               const std::string& value) = 0;
    virtual void RemovePreference(const std::string& name) = 0;
    virtual void ClearPreferences() = 0;
    virtual void RegisterExtensionsAPI(const std::string& origin,
                                       const std::string& script) = 0;
  };

  using DispatchCallback = Delegate::DispatchCallback;

  virtual ~DevToolsEmbedderMessageDispatcher() {}
  virtual bool Dispatch(const DispatchCallback& callback,
                        const std::string& method,
                        const base::ListValue* params) = 0;

  static DevToolsEmbedderMessageDispatcher* CreateForDevToolsFrontend(
      Delegate* delegate);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_DEVTOOLS_EMBEDDER_MESSAGE_DISPATCHER_H_
