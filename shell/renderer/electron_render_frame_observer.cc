// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_render_frame_observer.h"

#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/options_switches.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/renderer_client_base.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_draggable_region.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace electron {

namespace {

scoped_refptr<base::RefCountedMemory> NetResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
        IDR_DIR_HEADER_HTML);
  }
  return nullptr;
}

}  // namespace

ElectronRenderFrameObserver::ElectronRenderFrameObserver(
    content::RenderFrame* frame,
    RendererClientBase* renderer_client)
    : content::RenderFrameObserver(frame),
      render_frame_(frame),
      renderer_client_(renderer_client) {
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

void ElectronRenderFrameObserver::DidClearWindowObject() {
  // Do a delayed Node.js initialization for child window.
  // Check DidInstallConditionalFeatures below for the background.
  auto* web_frame = render_frame_->GetWebFrame();
  if (has_delayed_node_initialization_ && web_frame->Opener() &&
      !web_frame->GetDocument().Url().IsEmpty()) {
    v8::Isolate* isolate = blink::MainThreadIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::MicrotasksScope microtasks_scope(
        isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
    v8::Handle<v8::Context> context =
        render_frame_->GetWebFrame()->MainWorldScriptContext();
    v8::Context::Scope context_scope(context);
    // DidClearWindowObject only emits for the main world.
    DidInstallConditionalFeatures(context, MAIN_WORLD_ID);
  }

  renderer_client_->DidClearWindowObject(render_frame_);
}

void ElectronRenderFrameObserver::DidInstallConditionalFeatures(
    v8::Handle<v8::Context> context,
    int world_id) {
  // When a child window is created with window.open, its WebPreferences will
  // be copied from its parent, and Chromium will load empty page in it
  // immediately.
  // Normally the WebPreferences is overriden in browser before navigation,
  // but this behavior bypasses the browser side navigation and the child
  // window will get wrong WebPreferences when loading empty page.
  // This will end up initializing Node.js in the child window with wrong
  // WebPreferences, leads to problem that child window having node integration
  // while "nodeIntegration=no" is passed.
  // We work around this issue by delaying the child window's initialization of
  // Node.js if an empty page is being loaded, and only do it when the acutal
  // page has started to load.
  auto* web_frame = render_frame_->GetWebFrame();
  if (web_frame->Opener() && web_frame->GetDocument().Url().IsEmpty()) {
    has_delayed_node_initialization_ = true;
    return;
  }
  has_delayed_node_initialization_ = false;

  auto* isolate = context->GetIsolate();
  v8::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);

  if (ShouldNotifyClient(world_id))
    renderer_client_->DidCreateScriptContext(context, render_frame_);

  auto prefs = render_frame_->GetBlinkPreferences();
  bool use_context_isolation = prefs.context_isolation;
  // This logic matches the EXPLAINED logic in electron_renderer_client.cc
  // to avoid explaining it twice go check that implementation in
  // DidCreateScriptContext();
  bool is_main_world = IsMainWorld(world_id);
  bool is_main_frame = render_frame_->IsMainFrame();
  bool allow_node_in_sub_frames = prefs.node_integration_in_sub_frames;

  bool should_create_isolated_context =
      use_context_isolation && is_main_world &&
      (is_main_frame || allow_node_in_sub_frames);

  if (should_create_isolated_context) {
    CreateIsolatedWorldContext();
    if (!renderer_client_->IsWebViewFrame(context, render_frame_))
      renderer_client_->SetupMainWorldOverrides(context, render_frame_);
  }
}

void ElectronRenderFrameObserver::DraggableRegionsChanged() {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      render_frame_->GetWebFrame()->GetDocument().DraggableRegions();
  std::vector<mojom::DraggableRegionPtr> regions;
  for (auto& webregion : webregions) {
    auto region = mojom::DraggableRegion::New();
    render_frame_->ConvertViewportToWindow(&webregion.bounds);
    region->bounds = webregion.bounds;
    region->draggable = webregion.draggable;
    regions.push_back(std::move(region));
  }

  mojo::Remote<mojom::ElectronBrowser> browser_remote;
  render_frame_->GetBrowserInterfaceBroker()->GetInterface(
      browser_remote.BindNewPipeAndPassReceiver());
  browser_remote->UpdateDraggableRegions(std::move(regions));
}

void ElectronRenderFrameObserver::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int world_id) {
  if (ShouldNotifyClient(world_id))
    renderer_client_->WillReleaseScriptContext(context, render_frame_);
}

void ElectronRenderFrameObserver::OnDestruct() {
  delete this;
}

void ElectronRenderFrameObserver::DidMeaningfulLayout(
    blink::WebMeaningfulLayout layout_type) {
  if (layout_type == blink::WebMeaningfulLayout::kVisuallyNonEmpty) {
    mojo::Remote<mojom::ElectronBrowser> browser_remote;
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        browser_remote.BindNewPipeAndPassReceiver());
    browser_remote->OnFirstNonEmptyLayout();
  }
}

void ElectronRenderFrameObserver::CreateIsolatedWorldContext() {
  auto* frame = render_frame_->GetWebFrame();
  blink::WebIsolatedWorldInfo info;
  // This maps to the name shown in the context combo box in the Console tab
  // of the dev tools.
  info.human_readable_name =
      blink::WebString::FromUTF8("Electron Isolated Context");
  // Setup document's origin policy in isolated world
  info.security_origin = frame->GetDocument().GetSecurityOrigin();
  blink::SetIsolatedWorldInfo(WorldIDs::ISOLATED_WORLD_ID, info);

  // Create initial script context in isolated world
  blink::WebScriptSource source("void 0");
  frame->ExecuteScriptInIsolatedWorld(
      WorldIDs::ISOLATED_WORLD_ID, source,
      blink::BackForwardCacheAware::kPossiblyDisallow);
}

bool ElectronRenderFrameObserver::IsMainWorld(int world_id) {
  return world_id == WorldIDs::MAIN_WORLD_ID;
}

bool ElectronRenderFrameObserver::IsIsolatedWorld(int world_id) {
  return world_id == WorldIDs::ISOLATED_WORLD_ID;
}

bool ElectronRenderFrameObserver::ShouldNotifyClient(int world_id) {
  auto prefs = render_frame_->GetBlinkPreferences();

  // This is necessary because if an iframe is created and a source is not
  // set, the iframe loads about:blank and creates a script context for the
  // same. We don't want to create a Node.js environment here because if the src
  // is later set, the JS necessary to do that triggers illegal access errors
  // when the initial about:blank Node.js environment is cleaned up. See:
  // https://source.chromium.org/chromium/chromium/src/+/main:content/renderer/render_frame_impl.h;l=870-892;drc=4b6001440a18740b76a1c63fa2a002cc941db394
  GURL url = render_frame_->GetWebFrame()->GetDocument().Url();
  bool allow_node_in_sub_frames = prefs.node_integration_in_sub_frames;
  if (allow_node_in_sub_frames && url.IsAboutBlank() &&
      !render_frame_->IsMainFrame())
    return false;

  if (prefs.context_isolation &&
      (render_frame_->IsMainFrame() || allow_node_in_sub_frames))
    return IsIsolatedWorld(world_id);

  return IsMainWorld(world_id);
}

}  // namespace electron
