// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/renderer/brave_content_renderer_client.h"

#include "atom/browser/web_contents_preferences.h"
#include "atom/renderer/content_settings_client.h"
#include "atom/renderer/content_settings_manager.h"
#include "chrome/renderer/pepper/pepper_helper.h"
#include "content/public/renderer/render_thread.h"
#if defined(ENABLE_EXTENSIONS)
#include "atom/renderer/extensions/atom_extensions_renderer_client.h"
#include "atom/common/extensions/atom_extensions_client.h"
#include "extensions/renderer/dispatcher.h"
#endif

namespace brave {

BraveContentRendererClient::BraveContentRendererClient() {
#if defined(ENABLE_EXTENSIONS)
  extensions::ExtensionsClient::Set(
      extensions::AtomExtensionsClient::GetInstance());
  extensions::ExtensionsRendererClient::Set(
      extensions::AtomExtensionsRendererClient::GetInstance());
#endif
}

void BraveContentRendererClient::RenderThreadStarted() {
  content_settings_manager_ = atom::ContentSettingsManager::GetInstance();
  AtomRendererClient::RenderThreadStarted();
#if defined(ENABLE_EXTENSIONS)
  extensions::AtomExtensionsRendererClient::GetInstance()->
      RenderThreadStarted();
#endif
}

void BraveContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  extensions::Dispatcher* ext_dispatcher = NULL;
#if defined(ENABLE_EXTENSIONS)
  ext_dispatcher =
      extensions::AtomExtensionsRendererClient::GetInstance()->extension_dispatcher();
#endif
  new atom::ContentSettingsClient(render_frame,
                                  ext_dispatcher,
                                  content_settings_manager_);
#if defined(ENABLE_EXTENSIONS)
  extensions::AtomExtensionsRendererClient::GetInstance()->RenderFrameCreated(
    render_frame);
#endif
  if (atom::WebContentsPreferences::run_node()) {
    AtomRendererClient::RenderFrameCreated(render_frame);
  } else {
    new PepperHelper(render_frame);
  }
}

void BraveContentRendererClient::RenderViewCreated(content::RenderView* render_view) {
  AtomRendererClient::RenderViewCreated(render_view);
#if defined(ENABLE_EXTENSIONS)
  extensions::AtomExtensionsRendererClient::GetInstance()->
      RenderViewCreated(render_view);
#endif
}

void BraveContentRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
#if defined(ENABLE_EXTENSIONS)
  extensions::AtomExtensionsRendererClient::GetInstance()->
      RunScriptsAtDocumentStart(render_frame);
#endif
  if (atom::WebContentsPreferences::run_node())
    AtomRendererClient::RunScriptsAtDocumentStart(render_frame);
}

void BraveContentRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
#if defined(ENABLE_EXTENSIONS)
  extensions::AtomExtensionsRendererClient::GetInstance()->
      RunScriptsAtDocumentEnd(render_frame);
#endif
  if (atom::WebContentsPreferences::run_node())
    AtomRendererClient::RunScriptsAtDocumentEnd(render_frame);
}

bool BraveContentRendererClient::AllowPopup() {
  if (atom::WebContentsPreferences::run_node()) {
    return false;  // TODO(bridiver) - should return setting for allow popups
  }

#if defined(ENABLE_EXTENSIONS)
  return extensions::AtomExtensionsRendererClient::GetInstance()->AllowPopup();
#else
  return false;
#endif
}

bool BraveContentRendererClient::ShouldFork(blink::WebLocalFrame* frame,
                                    const GURL& url,
                                    const std::string& http_method,
                                    bool is_initial_navigation,
                                    bool is_server_redirect,
                                    bool* send_referrer) {
  if (atom::WebContentsPreferences::run_node()) {
    AtomRendererClient::ShouldFork(frame, url, http_method,
        is_initial_navigation, is_server_redirect, send_referrer);
  }

  return false;
}

bool BraveContentRendererClient::WillSendRequest(
    blink::WebFrame* frame,
    ui::PageTransition transition_type,
    const GURL& url,
    const GURL& first_party_for_cookies,
    GURL* new_url) {
  // Check whether the request should be allowed. If not allowed, we reset the
  // URL to something invalid to prevent the request and cause an error.
#if defined(ENABLE_EXTENSIONS)
  if (extensions::AtomExtensionsRendererClient::GetInstance()->WillSendRequest(
          frame, transition_type, url, new_url))
    return true;
#endif

  return false;
}

void BraveContentRendererClient::DidInitializeServiceWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context,
    const GURL& url) {
#if defined(ENABLE_EXTENSIONS)
  extensions::Dispatcher::DidInitializeServiceWorkerContextOnWorkerThread(
      context, url);
#endif
}

void BraveContentRendererClient::WillDestroyServiceWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context,
    const GURL& url) {
#if defined(ENABLE_EXTENSIONS)
  extensions::Dispatcher::WillDestroyServiceWorkerContextOnWorkerThread(context,
                                                                        url);
#endif
}

}  // namespace brave
