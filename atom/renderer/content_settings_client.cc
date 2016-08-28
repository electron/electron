// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/content_settings_client.h"

#include <string>

#include "atom/renderer/content_settings_manager.h"
#include "atom/common/api/api_messages.h"
#include "base/strings/utf_string_conversions.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/platform/URLConversion.h"
#include "third_party/WebKit/public/platform/WebContentSettingCallbacks.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrameClient.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "url/url_constants.h"

#if defined(ENABLE_EXTENSIONS)
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/renderer_extension_registry.h"
#endif

using blink::WebContentSettingCallbacks;
using blink::WebDataSource;
using blink::WebDocument;
using blink::WebFrame;
using blink::WebSecurityOrigin;
using blink::WebString;
using blink::WebURL;
using blink::WebView;
using content::DocumentState;
using content::NavigationState;

namespace {

GURL GetOriginOrURL(const WebFrame* frame) {
  WebString top_origin = frame->top()->getSecurityOrigin().toString();
  // The |top_origin| is unique ("null") e.g., for file:// URLs. Use the
  // document URL as the primary URL in those cases.
  // TODO(alexmos): This is broken for --site-per-process, since top() can be a
  // WebRemoteFrame which does not have a document(), and the WebRemoteFrame's
  // URL is not replicated.
  if (top_origin == "null")
    return frame->top()->document().url();
  return blink::WebStringToGURL(top_origin);
}

}  // namespace

namespace atom {

ContentSettingsClient::ContentSettingsClient(
    content::RenderFrame* render_frame,
    extensions::Dispatcher* extension_dispatcher,
    ContentSettingsManager* content_settings_manager)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<ContentSettingsClient>(
          render_frame),
#if defined(ENABLE_EXTENSIONS)
      extension_dispatcher_(extension_dispatcher),
#endif
      content_settings_manager_(
        content_settings_manager) {
  // this has to be set or FromString errors out
  ContentSettingsPattern::SetNonWildcardDomainNonPortScheme(
    extensions::kExtensionScheme);

  ClearBlockedContentSettings();
  render_frame->GetWebFrame()->setContentSettingsClient(this);
}

ContentSettingsClient::~ContentSettingsClient() {
}

void ContentSettingsClient::DidBlockContentType(
    const std::string& settings_type) {
  DidBlockContentType(settings_type,
      blink::WebStringToGURL(render_frame()->GetWebFrame()->
          getSecurityOrigin().toString()).spec());
}

void ContentSettingsClient::DidBlockContentType(
    const std::string& settings_type,
    const std::string& details) {
  base::ListValue args;
  args.AppendString(settings_type);
  args.AppendString(details);

  auto rv = render_frame()->GetRenderView();
  rv->Send(new AtomViewHostMsg_Message(
    rv->GetRoutingID(), base::UTF8ToUTF16("content-blocked"), args));
}

void ContentSettingsClient::DidCommitProvisionalLoad(
    bool is_new_navigation,
    bool is_same_page_navigation) {
  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->parent())
    return;  // Not a top-level navigation.

  if (!is_same_page_navigation) {
    ClearBlockedContentSettings();
  }
}

bool ContentSettingsClient::allowDatabase(const WebString& name,
                                          const WebString& display_name,
                                          unsigned long estimated_size) {  // NOLINT
  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique())
    return false;

  bool allow = true;
  GURL secondary_url(
      blink::WebStringToGURL(frame->getSecurityOrigin().toString()));
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          GetOriginOrURL(frame),
          secondary_url,
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("database", secondary_url.spec());
  return allow;
}


void ContentSettingsClient::requestFileSystemAccessAsync(
        const WebContentSettingCallbacks& callbacks) {
  WebFrame* frame = render_frame()->GetWebFrame();
  WebContentSettingCallbacks permissionCallbacks(callbacks);
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique()) {
      permissionCallbacks.doDeny();
      return;
  }

  bool allow = true;
  GURL secondary_url(
      blink::WebStringToGURL(frame->getSecurityOrigin().toString()));
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          GetOriginOrURL(frame),
          secondary_url,
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }
  if (!allow) {
      DidBlockContentType("filesystem", secondary_url.spec());
      permissionCallbacks.doDeny();
  } else {
      permissionCallbacks.doAllow();
  }
}

bool ContentSettingsClient::allowImage(bool enabled_per_settings,
                                         const WebURL& image_url) {
  if (enabled_per_settings && IsWhitelistedForContentSettings())
    return true;

  bool allow = enabled_per_settings;
  GURL secondary_url(image_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
                                 GetOriginOrURL(render_frame()->GetWebFrame()),
                                 secondary_url,
                                 "images",
                                 allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("images", secondary_url.spec());
  return allow;
}

bool ContentSettingsClient::allowIndexedDB(const WebString& name,
                                             const WebSecurityOrigin& origin) {
  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique())
    return false;

  bool allow = true;
  GURL secondary_url(
      blink::WebStringToGURL(frame->getSecurityOrigin().toString()));
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          GetOriginOrURL(frame),
          secondary_url,
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("indexedDB", secondary_url.spec());
  return allow;
}

bool ContentSettingsClient::allowPlugins(bool enabled_per_settings) {
  return enabled_per_settings;
}

bool ContentSettingsClient::allowScript(bool enabled_per_settings) {
  if (IsWhitelistedForContentSettings())
    return true;

  WebFrame* frame = render_frame()->GetWebFrame();
  std::map<WebFrame*, bool>::const_iterator it =
      cached_script_permissions_.find(frame);
  if (it != cached_script_permissions_.end())
    return it->second;

  bool allow = enabled_per_settings;
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
                                   GetOriginOrURL(frame),
                                   GURL(),
                                   "javascript",
                                   allow) != CONTENT_SETTING_BLOCK;
  }

  cached_script_permissions_[frame] = allow;
  if (!allow)
    DidBlockContentType("javascript");
  return allow;
}

bool ContentSettingsClient::allowScriptFromSource(
    bool enabled_per_settings,
    const blink::WebURL& script_url) {
  if (IsWhitelistedForContentSettings())
    return true;

  bool allow = enabled_per_settings;
  GURL secondary_url(script_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
                                 GetOriginOrURL(render_frame()->GetWebFrame()),
                                 secondary_url,
                                 "javascript",
                                 allow) != CONTENT_SETTING_BLOCK;
  }

  allow = allow || IsWhitelistedForContentSettings();
  if (!allow)
    DidBlockContentType("javascript", secondary_url.spec());
  return allow;
}

bool ContentSettingsClient::allowStorage(bool local) {
  if (IsWhitelistedForContentSettings())
    return true;

  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->getSecurityOrigin().isUnique() ||
      frame->top()->getSecurityOrigin().isUnique())
    return false;

  StoragePermissionsKey key(
      blink::WebStringToGURL(frame->document().getSecurityOrigin().toString()),
      local);
  std::map<StoragePermissionsKey, bool>::const_iterator permissions =
      cached_storage_permissions_.find(key);
  if (permissions != cached_storage_permissions_.end())
    return permissions->second;

  bool allow = true;
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          GetOriginOrURL(frame),
          blink::WebStringToGURL(frame->getSecurityOrigin().toString()),
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }

  cached_storage_permissions_[key] = allow;
  if (!allow)
    DidBlockContentType("storage");
  return allow;
}

bool ContentSettingsClient::allowReadFromClipboard(bool default_value) {
  bool allowed = default_value;
#if defined(ENABLE_EXTENSIONS)
  extensions::ScriptContext* current_context =
      extension_dispatcher_->script_context_set().GetCurrent();
  if (current_context) {
    allowed |= current_context->HasAPIPermission(
        extensions::APIPermission::kClipboardRead);
  }
#endif
  return allowed;
}

bool ContentSettingsClient::allowWriteToClipboard(bool default_value) {
  bool allowed = default_value;
#if defined(ENABLE_EXTENSIONS)
  // All blessed extension pages could historically write to the clipboard, so
  // preserve that for compatibility.
  extensions::ScriptContext* current_context =
      extension_dispatcher_->script_context_set().GetCurrent();
  if (current_context) {
    if (current_context->effective_context_type() ==
        extensions::Feature::BLESSED_EXTENSION_CONTEXT) {
      allowed = true;
    } else {
      allowed |= current_context->HasAPIPermission(
          extensions::APIPermission::kClipboardWrite);
    }
  }
#endif
  return allowed;
}

bool ContentSettingsClient::allowMutationEvents(bool default_value) {
  if (IsWhitelistedForContentSettings())
    return true;

  bool allow = default_value;
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
                                 GetOriginOrURL(render_frame()->GetWebFrame()),
                                 GURL(),
                                 "mutation",
                                 allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("mutation");
  return allow;
}

bool ContentSettingsClient::allowDisplayingInsecureContent(
    bool allowed_per_settings,
    const blink::WebURL& resource_url) {

  bool allow = allowed_per_settings;
  GURL secondary_url(resource_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
                                 GetOriginOrURL(render_frame()->GetWebFrame()),
                                 secondary_url,
                                 "displayInsecureContent",
                                 allow) != CONTENT_SETTING_BLOCK;
  }

  if (allow)
    DidDisplayInsecureContent(GURL(resource_url));
  else
    DidBlockDisplayInsecureContent(GURL(resource_url));
  return allow;
}

bool ContentSettingsClient::allowRunningInsecureContent(
    bool allowed_per_settings,
    const blink::WebSecurityOrigin& origin,
    const blink::WebURL& resource_url) {
  // TODO(bridiver) is origin different than web frame top origin?
  bool allow = allowed_per_settings;
  GURL secondary_url(resource_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
                                 GetOriginOrURL(render_frame()->GetWebFrame()),
                                 secondary_url,
                                 "runInsecureContent",
                                 allow) != CONTENT_SETTING_BLOCK;
  }

  if (allow)
    DidRunInsecureContent(GURL(resource_url));
  else
    DidBlockRunInsecureContent(GURL(resource_url));
  return allow;
}

void ContentSettingsClient::DidDisplayInsecureContent(GURL resource_url) {
  base::ListValue args;
  args.AppendString(resource_url.spec());

  auto rv = render_frame()->GetRenderView();
  rv->Send(new AtomViewHostMsg_Message(rv->GetRoutingID(),
      base::UTF8ToUTF16("did-display-insecure-content"), args));
}

void ContentSettingsClient::DidRunInsecureContent(GURL resouce_url) {
  base::ListValue args;
    args.AppendString(resouce_url.spec());

    auto rv = render_frame()->GetRenderView();
    rv->Send(new AtomViewHostMsg_Message(rv->GetRoutingID(),
        base::UTF8ToUTF16("did-run-insecure-content"), args));
}

void ContentSettingsClient::DidBlockDisplayInsecureContent(GURL resource_url) {
  base::ListValue args;
  args.AppendString(resource_url.spec());

  auto rv = render_frame()->GetRenderView();
  rv->Send(new AtomViewHostMsg_Message(rv->GetRoutingID(),
      base::UTF8ToUTF16("did-block-display-insecure-content"), args));
}

void ContentSettingsClient::DidBlockRunInsecureContent(GURL resouce_url) {
  base::ListValue args;
    args.AppendString(resouce_url.spec());

    auto rv = render_frame()->GetRenderView();
    rv->Send(new AtomViewHostMsg_Message(rv->GetRoutingID(),
        base::UTF8ToUTF16("did-block-run-insecure-content"), args));
}

void ContentSettingsClient::ClearBlockedContentSettings() {
  cached_storage_permissions_.clear();
  cached_script_permissions_.clear();
}

bool ContentSettingsClient::IsWhitelistedForContentSettings() const {
  // Whitelist ftp directory listings, as they require JavaScript to function
  // properly.
  if (render_frame()->IsFTPDirectoryListing())
    return true;

  WebFrame* web_frame = render_frame()->GetWebFrame();
  return IsWhitelistedForContentSettings(
      web_frame->document().getSecurityOrigin(), web_frame->document().url());
}

bool ContentSettingsClient::IsWhitelistedForContentSettings(
    const WebSecurityOrigin& origin,
    const GURL& document_url) {
  if (document_url == GURL(content::kUnreachableWebDataURL))
    return true;

  if (origin.isUnique())
    return false;  // Uninitialized document?

  base::string16 protocol = origin.protocol();
  if (base::EqualsASCII(protocol, content::kChromeUIScheme))
    return true;  // Browser UI elements should still work.

  if (base::EqualsASCII(protocol, content::kChromeDevToolsScheme))
    return true;  // DevTools UI elements should still work.

#if defined(ENABLE_EXTENSIONS)
  if (base::EqualsASCII(protocol, extensions::kExtensionScheme))
    return true;
#endif

  // If the scheme is file:, an empty file name indicates a directory listing,
  // which requires JavaScript to function properly.
  if (base::EqualsASCII(protocol, url::kFileScheme)) {
    return document_url.SchemeIs(url::kFileScheme) &&
           document_url.ExtractFileName().empty();
  }

  return false;
}

}  // namespace atom
