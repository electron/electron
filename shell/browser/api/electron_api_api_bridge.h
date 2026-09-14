// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_API_BRIDGE_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_API_BRIDGE_H_

#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom-forward.h"

namespace content {
class RenderFrameHost;
class WebContents;
}  // namespace content

// The browser side of apiBridge: APIs the main process passes to frames with
// frame.apiBridge.pass() and session.apiBridge.pass().
//
// A grant belongs to one FrameTreeNode (frame.apiBridge.pass) or to every frame
// of a session (session.apiBridge.pass), to a world, and to a list of origins.
// It is pushed to every matching document ahead of its commit, so it is on
// navigator.electron before any script runs. Every call from a renderer is
// checked against the grants the calling document should have and its
// committed origin.
namespace electron::api::api_bridge {

// Follows the navigations and frames of |web_contents|, so its documents get
// their grants. Call it before the first navigation; calling it again is a
// no-op.
void ObserveWebContents(content::WebContents* web_contents);

// Binds the ElectronApiBridgeHost a frame's renderer calls into.
void BindHost(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiBridgeHost> receiver);

}  // namespace electron::api::api_bridge

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_API_BRIDGE_H_
