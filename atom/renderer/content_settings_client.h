// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_CONTENT_SETTINGS_CLIENT_H_
#define ATOM_RENDERER_CONTENT_SETTINGS_CLIENT_H_

#include <map>
#include <set>
#include <utility>
#include <string>
#include "atom/renderer/content_settings_observer.h"
#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "third_party/WebKit/public/web/WebContentSettingsClient.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace blink {
class WebFrame;
class WebSecurityOrigin;
class WebURL;
}

namespace extensions {
class Dispatcher;
}

namespace atom {

class ContentSettingsManager;

// Handles blocking content per content settings for each RenderFrame.
class ContentSettingsClient
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<ContentSettingsClient>,
      public blink::WebContentSettingsClient {
 public:
  ContentSettingsClient(content::RenderFrame* render_frame,
                          extensions::Dispatcher* extension_dispatcher,
                          ContentSettingsManager* content_settings_manager);
  ~ContentSettingsClient() override;

  // Sends an IPC notification that the specified content type was blocked.
  void DidBlockContentType(const std::string& settings_type);

  // Sends an IPC notification that the specified content type was blocked
  // with additional metadata.
  void DidBlockContentType(const std::string& settings_type,
                           const std::string& details);

  // blink::WebContentSettingsClient implementation.
  bool allowDatabase(const blink::WebString& name,
                     const blink::WebString& display_name,
                     unsigned long estimated_size) override;  // NOLINT
  void requestFileSystemAccessAsync(
          const blink::WebContentSettingCallbacks& callbacks) override;
  bool allowImage(bool enabled_per_settings,
                  const blink::WebURL& image_url) override;
  bool allowIndexedDB(const blink::WebString& name,
                      const blink::WebSecurityOrigin& origin) override;
  bool allowPlugins(bool enabled_per_settings) override;
  bool allowScript(bool enabled_per_settings) override;
  bool allowScriptFromSource(bool enabled_per_settings,
                             const blink::WebURL& script_url) override;
  bool allowStorage(bool local) override;
  bool allowReadFromClipboard(bool default_value) override;
  bool allowWriteToClipboard(bool default_value) override;
  bool allowMutationEvents(bool default_value) override;
  bool allowDisplayingInsecureContent(bool allowed_per_settings,
                                      const blink::WebURL& url) override;
  bool allowRunningInsecureContent(bool allowed_per_settings,
                                   const blink::WebSecurityOrigin& context,
                                   const blink::WebURL& url) override;

 private:
  void DidDisplayInsecureContent(GURL resource_url);
  void DidRunInsecureContent(GURL resouce_url);
  void DidBlockDisplayInsecureContent(GURL resource_url);
  void DidBlockRunInsecureContent(GURL resouce_url);

  // RenderFrameObserver implementation.
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_page_navigation) override;

  // Resets the |content_blocked_| array.
  void ClearBlockedContentSettings();

  // Helpers.
  // True if |render_frame()| contains content that is white-listed for content
  // settings.
  bool IsWhitelistedForContentSettings() const;
  static bool IsWhitelistedForContentSettings(
      const blink::WebSecurityOrigin& origin,
      const GURL& document_url);

#if defined(ENABLE_EXTENSIONS)
  // Owned by ChromeContentRendererClient and outlive us.
  extensions::Dispatcher* extension_dispatcher_;
#endif
  ContentSettingsManager* content_settings_manager_;  // not owned

  // Caches the result of AllowStorage.
  typedef std::pair<GURL, bool> StoragePermissionsKey;
  std::map<StoragePermissionsKey, bool> cached_storage_permissions_;

  // Caches the result of AllowScript.
  std::map<blink::WebFrame*, bool> cached_script_permissions_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_CONTENT_SETTINGS_CLIENT_H_
