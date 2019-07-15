// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/atom_render_frame_observer.h"

#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "electron/shell/common/api/api.mojom.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/common/api/event_emitter_caller.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
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

base::StringPiece NetResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    base::StringPiece html_data =
        ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_DIR_HEADER_HTML);
    return html_data;
  }
  return base::StringPiece();
}

}  // namespace

AtomRenderFrameObserver::AtomRenderFrameObserver(
    content::RenderFrame* frame,
    RendererClientBase* renderer_client)
    : content::RenderFrameObserver(frame),
      render_frame_(frame),
      renderer_client_(renderer_client) {
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

void AtomRenderFrameObserver::DidClearWindowObject() {
  renderer_client_->DidClearWindowObject(render_frame_);
}

void AtomRenderFrameObserver::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    int world_id) {
  if (ShouldNotifyClient(world_id))
    renderer_client_->DidCreateScriptContext(context, render_frame_);

  bool use_context_isolation = renderer_client_->isolated_world();
  bool is_main_world = IsMainWorld(world_id);
  bool is_main_frame = render_frame_->IsMainFrame();
  bool is_not_opened = !render_frame_->GetWebFrame()->Opener();
  bool allow_node_in_sub_frames =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInSubFrames);
  bool should_create_isolated_context =
      use_context_isolation && is_main_world &&
      (is_main_frame || allow_node_in_sub_frames) && is_not_opened;

  if (should_create_isolated_context) {
    CreateIsolatedWorldContext();
    renderer_client_->SetupMainWorldOverrides(context, render_frame_);
  }

  if (world_id >= World::ISOLATED_WORLD_EXTENSIONS &&
      world_id <= World::ISOLATED_WORLD_EXTENSIONS_END) {
    renderer_client_->SetupExtensionWorldOverrides(context, render_frame_,
                                                   world_id);
  }
}

void AtomRenderFrameObserver::DraggableRegionsChanged() {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      render_frame_->GetWebFrame()->GetDocument().DraggableRegions();
  std::vector<mojom::DraggableRegionPtr> regions;
  for (auto& webregion : webregions) {
    auto region = mojom::DraggableRegion::New();
    render_frame_->GetRenderView()->ConvertViewportToWindowViaWidget(
        &webregion.bounds);
    region->bounds = webregion.bounds;
    region->draggable = webregion.draggable;
    regions.push_back(std::move(region));
  }

  mojom::ElectronBrowserPtr browser_ptr;
  render_frame_->GetRemoteInterfaces()->GetInterface(
      mojo::MakeRequest(&browser_ptr));
  browser_ptr->UpdateDraggableRegions(std::move(regions));
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
  auto* frame = render_frame_->GetWebFrame();
  blink::WebIsolatedWorldInfo info;
  // This maps to the name shown in the context combo box in the Console tab
  // of the dev tools.
  info.human_readable_name =
      blink::WebString::FromUTF8("Electron Isolated Context");
  // Setup document's origin policy in isolated world
  info.security_origin = frame->GetDocument().GetSecurityOrigin();
  frame->SetIsolatedWorldInfo(World::ISOLATED_WORLD, info);

  // Create initial script context in isolated world
  blink::WebScriptSource source("void 0");
  frame->ExecuteScriptInIsolatedWorld(World::ISOLATED_WORLD, source);
}

bool AtomRenderFrameObserver::IsMainWorld(int world_id) {
  return world_id == World::MAIN_WORLD;
}

bool AtomRenderFrameObserver::IsIsolatedWorld(int world_id) {
  return world_id == World::ISOLATED_WORLD;
}

bool AtomRenderFrameObserver::ShouldNotifyClient(int world_id) {
  bool allow_node_in_sub_frames =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInSubFrames);
  if (renderer_client_->isolated_world() &&
      (render_frame_->IsMainFrame() || allow_node_in_sub_frames))
    return IsIsolatedWorld(world_id);
  else
    return IsMainWorld(world_id);
}

}  // namespace electron
