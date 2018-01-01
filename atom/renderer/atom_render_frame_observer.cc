// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_frame_observer.h"

#include <vector>

#include "atom/common/api/api_messages.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"

namespace atom {

AtomRenderFrameObserver::AtomRenderFrameObserver(
    content::RenderFrame* frame,
    RendererClientBase* renderer_client)
  : content::RenderFrameObserver(frame),
    render_frame_(frame),
    renderer_client_(renderer_client) {}

void AtomRenderFrameObserver::DidClearWindowObject() {
  renderer_client_->DidClearWindowObject(render_frame_);
}

void AtomRenderFrameObserver::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    int world_id) {
  if (ShouldNotifyClient(world_id))
    renderer_client_->DidCreateScriptContext(context, render_frame_);

  if (renderer_client_->isolated_world() && IsMainWorld(world_id)
      && render_frame_->IsMainFrame()) {
    CreateIsolatedWorldContext();
    renderer_client_->SetupMainWorldOverrides(context);
  }
}

void AtomRenderFrameObserver::DraggableRegionsChanged() {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      render_frame_->GetWebFrame()->GetDocument().DraggableRegions();
  std::vector<DraggableRegion> regions;
  for (auto& webregion : webregions) {
    DraggableRegion region;
    render_frame_->GetRenderView()->ConvertViewportToWindowViaWidget(
        &webregion.bounds);
    region.bounds = webregion.bounds;
    region.draggable = webregion.draggable;
    regions.push_back(region);
  }
  Send(new AtomFrameHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

void AtomRenderFrameObserver::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int world_id) {
  if (ShouldNotifyClient(world_id))
    renderer_client_->WillReleaseScriptContext(context, render_frame_);
}

void AtomRenderFrameObserver::OnDestruct() {
  delete this;
}

void AtomRenderFrameObserver::CreateIsolatedWorldContext() {
  auto frame = render_frame_->GetWebFrame();

  // This maps to the name shown in the context combo box in the Console tab
  // of the dev tools.
  frame->SetIsolatedWorldHumanReadableName(
      World::ISOLATED_WORLD,
      blink::WebString::FromUTF8("Electron Isolated Context"));

  // Setup document's origin policy in isolated world
  frame->SetIsolatedWorldSecurityOrigin(
    World::ISOLATED_WORLD, frame->GetDocument().GetSecurityOrigin());

  // Create initial script context in isolated world
  blink::WebScriptSource source("void 0");
  frame->ExecuteScriptInIsolatedWorld(World::ISOLATED_WORLD, &source, 1);
}

bool AtomRenderFrameObserver::IsMainWorld(int world_id) {
  return world_id == World::MAIN_WORLD;
}

bool AtomRenderFrameObserver::IsIsolatedWorld(int world_id) {
  return world_id == World::ISOLATED_WORLD;
}

bool AtomRenderFrameObserver::ShouldNotifyClient(int world_id) {
  if (renderer_client_->isolated_world() && render_frame_->IsMainFrame())
    return IsIsolatedWorld(world_id);
  else
    return IsMainWorld(world_id);
}

}  // namespace atom
