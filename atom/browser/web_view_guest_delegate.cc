// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_view_guest_delegate.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/api/event.h"
#include "atom/browser/native_window.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/node_includes.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/guest_host.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace {

const int kDefaultWidth = 300;
const int kDefaultHeight = 300;

}  // namespace

WebViewGuestDelegate::WebViewGuestDelegate()
    : guest_host_(nullptr),
      auto_size_enabled_(false),
      is_full_page_plugin_(false),
      api_web_contents_(nullptr),
      guest_proxy_routing_id_(-1) {
}

WebViewGuestDelegate::~WebViewGuestDelegate() {
}

void WebViewGuestDelegate::Initialize(api::WebContents* api_web_contents) {
  api_web_contents_ = api_web_contents;
  Observe(api_web_contents->GetWebContents());
}

void WebViewGuestDelegate::Destroy() {
  // Give the content module an opportunity to perform some cleanup.
  guest_host_->WillDestroy();
  guest_host_ = nullptr;
}

content::WebContents* WebViewGuestDelegate::CreateNewGuestWindow(
                            const content::WebContents::CreateParams& params) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto isolate = api_web_contents_->isolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  // window options will come from features that needs to be passed through
  mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);
  options.Set("isGuest", true);

  int guest_instance_id = GetNextInstanceId();
  options.Set(options::kGuestInstanceID, guest_instance_id);

  // TODO(bridiver) should we create a new site instance
  // from web_contents_impl.cc
  // scoped_refptr<SiteInstance> site_instance =
  //     params.opener_suppressed && !is_guest
  //         ? SiteInstance::CreateForURL(GetBrowserContext(),
  //             params.target_url)
  //         : source_site_instance;

  if (params.site_instance) {
    auto session = atom::api::Session::CreateFrom(isolate,
            static_cast<AtomBrowserContext*>(
                params.site_instance->GetBrowserContext()));
    options.Set("session", session);
  }

  // get the underlying contents::WebContents object
  mate::Handle<api::WebContents> new_api_web_contents =
          api::WebContents::CreateWithParams(isolate, options, params);
  content::WebContents* web_contents = new_api_web_contents->GetWebContents();
  RegisterGuest(new_api_web_contents, guest_instance_id);

  return web_contents;
}

void WebViewGuestDelegate::RegisterGuest(mate::Handle<api::WebContents> guest,
    int guest_instance_id) {
  auto isolate = api_web_contents_->isolate();
  v8::HandleScope handle_scope(isolate);

  auto add_guest_event =
            v8::Local<v8::Object>::Cast(mate::Event::Create(isolate).ToV8());
  node::Environment* env = node::Environment::GetCurrent(isolate);
  mate::EmitEvent(isolate,
                  env->process_object(),
                  "ELECTRON_GUEST_VIEW_MANAGER_REGISTER_GUEST",
                  add_guest_event,
                  guest,
                  guest_instance_id);
}

int WebViewGuestDelegate::GetNextInstanceId() {
  auto isolate = api_web_contents_->isolate();
  v8::HandleScope handle_scope(isolate);

  // get the next guest id and assign it to options and webPreferences
  auto next_instance_id_event = v8::Local<v8::Object>::Cast(
                                          mate::Event::Create(isolate).ToV8());
  node::Environment* env = node::Environment::GetCurrent(isolate);
  mate::EmitEvent(isolate,
                  env->process_object(),
                  "ELECTRON_GUEST_VIEW_MANAGER_NEXT_INSTANCE_ID",
                  next_instance_id_event);
  return next_instance_id_event->Get(
                      mate::StringToV8(isolate, "returnValue"))->NumberValue();
}

void WebViewGuestDelegate::SetSize(const SetSizeParams& params) {
  bool enable_auto_size =
      params.enable_auto_size ? *params.enable_auto_size : auto_size_enabled_;
  gfx::Size min_size = params.min_size ? *params.min_size : min_auto_size_;
  gfx::Size max_size = params.max_size ? *params.max_size : max_auto_size_;

  if (params.normal_size)
    normal_size_ = *params.normal_size;

  min_auto_size_ = min_size;
  min_auto_size_.SetToMin(max_size);
  max_auto_size_ = max_size;
  max_auto_size_.SetToMax(min_size);

  enable_auto_size &= !min_auto_size_.IsEmpty() && !max_auto_size_.IsEmpty();

  auto rvh = web_contents()->GetRenderViewHost();
  if (enable_auto_size) {
    // Autosize is being enabled.
    rvh->EnableAutoResize(min_auto_size_, max_auto_size_);
    normal_size_.SetSize(0, 0);
  } else {
    // Autosize is being disabled.
    // Use default width/height if missing from partially defined normal size.
    if (normal_size_.width() && !normal_size_.height())
      normal_size_.set_height(GetDefaultSize().height());
    if (!normal_size_.width() && normal_size_.height())
      normal_size_.set_width(GetDefaultSize().width());

    gfx::Size new_size;
    if (!normal_size_.IsEmpty()) {
      new_size = normal_size_;
    } else if (!guest_size_.IsEmpty()) {
      new_size = guest_size_;
    } else {
      new_size = GetDefaultSize();
    }

    if (auto_size_enabled_) {
      // Autosize was previously enabled.
      rvh->DisableAutoResize(new_size);
      GuestSizeChangedDueToAutoSize(guest_size_, new_size);
    } else {
      // Autosize was already disabled.
      guest_host_->SizeContents(new_size);
    }

    guest_size_ = new_size;
  }

  auto_size_enabled_ = enable_auto_size;
}

void WebViewGuestDelegate::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->HasCommitted() && !navigation_handle->IsErrorPage()) {
    auto is_main_frame = navigation_handle->IsInMainFrame();
    auto url = navigation_handle->GetURL();
    api_web_contents_->Emit("load-commit", url, is_main_frame);
  }
}

void WebViewGuestDelegate::DidStartProvisionalLoadForFrame(
      content::RenderFrameHost* render_frame_host,
      const GURL& url,
      bool is_error_page,
      bool is_iframe_srcdoc) {
  api_web_contents_->Emit("load-start", url, !render_frame_host->GetParent(),
                                              is_error_page, is_iframe_srcdoc);
}

void WebViewGuestDelegate::DidCommitProvisionalLoadForFrame(
    content::RenderFrameHost* render_frame_host,
    const GURL& url, ui::PageTransition transition_type) {
  api_web_contents_->Emit("load-commit", url, !render_frame_host->GetParent());
}

bool WebViewGuestDelegate::IsAttached() {
  return guest_proxy_routing_id_ != -1;
}

void WebViewGuestDelegate::DidAttach(int guest_proxy_routing_id) {
  guest_proxy_routing_id_ = guest_proxy_routing_id;
  api_web_contents_->Emit("did-attach", guest_proxy_routing_id);
  api_web_contents_->ResumeLoadingCreatedWebContents();
}

content::WebContents* WebViewGuestDelegate::GetOwnerWebContents() const {
  return embedder_web_contents_;
}

void WebViewGuestDelegate::GuestSizeChanged(const gfx::Size& new_size) {
  if (!auto_size_enabled_)
    return;
  GuestSizeChangedDueToAutoSize(guest_size_, new_size);
  guest_size_ = new_size;
}

void WebViewGuestDelegate::SetGuestHost(content::GuestHost* guest_host) {
  guest_host_ = guest_host;
}

void WebViewGuestDelegate::DidDetach() {
  guest_proxy_routing_id_ = -1;
}

void WebViewGuestDelegate::WillAttach(
    content::WebContents* embedder_web_contents,
    int element_instance_id,
    bool is_full_page_plugin,
    const base::Closure& completion_callback) {
  embedder_web_contents_ = embedder_web_contents;
  is_full_page_plugin_ = is_full_page_plugin;
  // update the owner window
  auto relay = NativeWindowRelay::FromWebContents(embedder_web_contents_);
  if (relay)
    api_web_contents_->SetOwnerWindow(relay->window.get());
  completion_callback.Run();
}

void WebViewGuestDelegate::GuestSizeChangedDueToAutoSize(
    const gfx::Size& old_size, const gfx::Size& new_size) {
  api_web_contents_->Emit("size-changed",
                          old_size.width(), old_size.height(),
                          new_size.width(), new_size.height());
}

gfx::Size WebViewGuestDelegate::GetDefaultSize() const {
  if (is_full_page_plugin_) {
    // Full page plugins default to the size of the owner's viewport.
    return embedder_web_contents_->GetRenderWidgetHostView()
                                 ->GetVisibleViewportSize();
  } else {
    return gfx::Size(kDefaultWidth, kDefaultHeight);
  }
}

}  // namespace atom
