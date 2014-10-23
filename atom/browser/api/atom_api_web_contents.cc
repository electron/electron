// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

v8::Persistent<v8::ObjectTemplate> template_;

}  // namespace

WebContents::WebContents(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
}

WebContents::WebContents(const mate::Dictionary& options) {
  content::WebContents::CreateParams params(AtomBrowserContext::Get());
  bool is_guest;
  if (options.Get("isGuest", &is_guest) && is_guest)
    params.guest_delegate = this;

  storage_.reset(content::WebContents::Create(params));
  Observe(storage_.get());
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

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();

  base::ListValue args;
  args.AppendBoolean(is_main_frame);
  Emit("did-frame-finish-load", args);

  if (is_main_frame)
    Emit("did-finish-load");
}

void WebContents::DidStartLoading(content::RenderViewHost* render_view_host) {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading(content::RenderViewHost* render_view_host) {
  Emit("did-stop-loading");
}

bool WebContents::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebContents, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AtomViewHostMsg_Message_Sync,
                                    OnRendererMessageSync)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebContents::WebContentsDestroyed() {
  // The RenderViewDeleted was not called when the WebContents is destroyed.
  RenderViewDeleted(web_contents()->GetRenderViewHost());
  Emit("destroyed");
}

void WebContents::Destroy() {
  if (storage_) {
    Observe(nullptr);
    storage_.reset();
  }
}

bool WebContents::IsAlive() const {
  return web_contents() != NULL;
}

void WebContents::LoadURL(const GURL& url) {
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = content::PAGE_TRANSITION_TYPED;
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents()->GetController().LoadURLWithParams(params);
}

GURL WebContents::GetURL() const {
  return web_contents()->GetURL();
}

base::string16 WebContents::GetTitle() const {
  return web_contents()->GetTitle();
}

bool WebContents::IsLoading() const {
  return web_contents()->IsLoading();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents()->Stop();
}

void WebContents::Reload() {
  // Navigating to a URL would always restart the renderer process, we want this
  // because normal reloading will break our node integration.
  // This is done by AtomBrowserClient::ShouldSwapProcessesForNavigation.
  LoadURL(GetURL());
}

void WebContents::ReloadIgnoringCache() {
  Reload();
}

bool WebContents::CanGoBack() const {
  return web_contents()->GetController().CanGoBack();
}

bool WebContents::CanGoForward() const {
  return web_contents()->GetController().CanGoForward();
}

bool WebContents::CanGoToOffset(int offset) const {
  return web_contents()->GetController().CanGoToOffset(offset);
}

void WebContents::GoBack() {
  web_contents()->GetController().GoBack();
}

void WebContents::GoForward() {
  web_contents()->GetController().GoForward();
}

void WebContents::GoToIndex(int index) {
  web_contents()->GetController().GoToIndex(index);
}

void WebContents::GoToOffset(int offset) {
  web_contents()->GetController().GoToOffset(offset);
}

int WebContents::GetRoutingID() const {
  return web_contents()->GetRoutingID();
}

int WebContents::GetProcessID() const {
  return web_contents()->GetRenderProcessHost()->GetID();
}

bool WebContents::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContents::ExecuteJavaScript(const base::string16& code) {
  web_contents()->GetMainFrame()->ExecuteJavaScript(code);
}

bool WebContents::SendIPCMessage(const base::string16& channel,
                                 const base::ListValue& args) {
  return Send(new AtomViewMsg_Message(routing_id(), channel, args));
}

mate::ObjectTemplateBuilder WebContents::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  if (template_.IsEmpty())
    template_.Reset(isolate, mate::ObjectTemplateBuilder(isolate)
        .SetMethod("destroy", &WebContents::Destroy)
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
        .SetMethod("_executeJavaScript", &WebContents::ExecuteJavaScript)
        .SetMethod("_send", &WebContents::SendIPCMessage)
        .Build());

  return mate::ObjectTemplateBuilder(
      isolate, v8::Local<v8::ObjectTemplate>::New(isolate, template_));
}

void WebContents::OnRendererMessage(const base::string16& channel,
                                    const base::ListValue& args) {
  // webContents.emit(channel, new Event(), args...);
  Emit(base::UTF16ToUTF8(channel), args, web_contents(), NULL);
}

void WebContents::OnRendererMessageSync(const base::string16& channel,
                                        const base::ListValue& args,
                                        IPC::Message* message) {
  // webContents.emit(channel, new Event(sender, message), args...);
  Emit(base::UTF16ToUTF8(channel), args, web_contents(), message);
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  return mate::CreateHandle(isolate, new WebContents(web_contents));
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, const mate::Dictionary& options) {
  return mate::CreateHandle(isolate, new WebContents(options));
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("create", &atom::api::WebContents::Create);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_contents, Initialize)
