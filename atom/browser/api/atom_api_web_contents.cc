// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"

namespace atom {

namespace api {

WebContents::WebContents(content::WebContents* web_contents)
    : web_contents_(web_contents) {
  Observe(web_contents_);
}

WebContents::~WebContents() {
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  base::ListValue args;
  args.AppendInteger(render_view_host->GetProcess()->GetID());
  args.AppendInteger(render_view_host->GetRoutingID());
  Emit("render-view-deleted", args);
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  Emit("crashed");
}

void WebContents::DidFinishLoad(int64 frame_id,
                                const GURL& validated_url,
                                bool is_main_frame,
                                content::RenderViewHost* render_view_host) {
  Emit("did-finish-load");
}

void WebContents::DidStartLoading(content::RenderViewHost* render_view_host) {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading(content::RenderViewHost* render_view_host) {
  Emit("did-stop-loading");
}

void WebContents::WebContentsDestroyed(content::WebContents*) {
  // The RenderViewDeleted is not called when the WebContents is destroyed
  // directly.
  RenderViewDeleted(web_contents_->GetRenderViewHost());

  Emit("destroyed");
  web_contents_ = NULL;
}

bool WebContents::IsAlive() const {
  return web_contents_ != NULL;
}

void WebContents::LoadURL(const GURL& url) {
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = content::PAGE_TRANSITION_TYPED;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents_->GetController().LoadURLWithParams(params);
}

GURL WebContents::GetURL() const {
  return web_contents_->GetURL();
}

string16 WebContents::GetTitle() const {
  return web_contents_->GetTitle();
}

bool WebContents::IsLoading() const {
  return web_contents_->IsLoading();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents_->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents_->Stop();
}

void WebContents::Reload() {
  web_contents_->GetController().Reload(false);
}

void WebContents::ReloadIgnoringCache() {
  web_contents_->GetController().ReloadIgnoringCache(false);
}

bool WebContents::CanGoBack() const {
  return web_contents_->GetController().CanGoBack();
}

bool WebContents::CanGoForward() const {
  return web_contents_->GetController().CanGoForward();
}

bool WebContents::CanGoToOffset(int offset) const {
  return web_contents_->GetController().CanGoToOffset(offset);
}

void WebContents::GoBack() {
  web_contents_->GetController().GoBack();
}

void WebContents::GoForward() {
  web_contents_->GetController().GoForward();
}

void WebContents::GoToIndex(int index) {
  web_contents_->GetController().GoToIndex(index);
}

void WebContents::GoToOffset(int offset) {
  web_contents_->GetController().GoToOffset(offset);
}

int WebContents::GetRoutingID() const {
  return web_contents_->GetRoutingID();
}

int WebContents::GetProcessID() const {
  return web_contents_->GetRenderProcessHost()->GetID();
}

bool WebContents::IsCrashed() const {
  return web_contents_->IsCrashed();
}

void WebContents::ExecuteJavaScript(const string16& code) {
  web_contents_->GetRenderViewHost()->ExecuteJavascriptInWebFrame(
      string16(), code);
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("isAlive", &WebContents::IsAlive)
      .SetMethod("loadUrl", &WebContents::LoadURL)
      .SetMethod("getUrl", &WebContents::GetURL)
      .SetMethod("getTitle", &WebContents::GetTitle)
      .SetMethod("isLoading", &WebContents::IsLoading)
      .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
      .SetMethod("stop", &WebContents::Stop)
      .SetMethod("reload", &WebContents::Reload)
      .SetMethod("reloadIgnoringCache", &WebContents::ReloadIgnoringCache)
      .SetMethod("canGoBack", &WebContents::CanGoBack)
      .SetMethod("canGoForward", &WebContents::CanGoForward)
      .SetMethod("canGoToOffset", &WebContents::CanGoToOffset)
      .SetMethod("goBack", &WebContents::GoBack)
      .SetMethod("goForward", &WebContents::GoForward)
      .SetMethod("goToIndex", &WebContents::GoToIndex)
      .SetMethod("goToOffset", &WebContents::GoToOffset)
      .SetMethod("getRoutingId", &WebContents::GetRoutingID)
      .SetMethod("getProcessId", &WebContents::GetProcessID)
      .SetMethod("isCrashed", &WebContents::IsCrashed)
      .SetMethod("executeJavaScript", &WebContents::ExecuteJavaScript);
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  return CreateHandle(isolate, new WebContents(web_contents));
}

}  // namespace api

}  // namespace atom
