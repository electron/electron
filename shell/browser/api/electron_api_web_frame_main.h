// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_

#include <optional>
#include <string>
#include <vector>

#include "base/process/process.h"
#include "base/values.h"
#include "content/public/browser/frame_tree_node_id.h"
#include "content/public/browser/global_routing_id.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-forward.h"

class GURL;

namespace content {
class RenderFrameHost;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
template <typename T>
class Promise;
}  // namespace gin_helper

namespace electron::api {

class WebContents;

// Bindings for accessing frames from the main process.
class WebFrameMain final : public gin::Wrappable<WebFrameMain>,
                           public gin_helper::EventEmitterMixin<WebFrameMain>,
                           public gin_helper::Constructible<WebFrameMain> {
 public:
  // Create a new WebFrameMain and return the V8 wrapper of it.
  static WebFrameMain* New(v8::Isolate* isolate);

  static WebFrameMain* From(v8::Isolate* isolate,
                            content::RenderFrameHost* render_frame_host);
  static WebFrameMain* FromFrameTreeNodeId(
      content::FrameTreeNodeId frame_tree_node_id);
  static WebFrameMain* FromFrameToken(
      content::GlobalRenderFrameHostToken frame_token);
  static WebFrameMain* FromRenderFrameHost(
      content::RenderFrameHost* render_frame_host);

  // gin_helper::Constructible
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "WebFrameMain"; }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  content::RenderFrameHost* render_frame_host() const;

  cppgc::Persistent<gin::WeakCell<WebFrameMain>> GetWeakCell();

  explicit WebFrameMain(content::RenderFrameHost* render_frame);
  ~WebFrameMain() override;

  // disable copy
  WebFrameMain(const WebFrameMain&) = delete;
  WebFrameMain& operator=(const WebFrameMain&) = delete;

 private:
  friend class WebContents;

  // Called when FrameTreeNode is deleted.
  void Destroyed();

  // Mark RenderFrameHost as disposed and to no longer access it. This can
  // happen when the WebFrameMain v8-forward.handle is GC'd or when a
  // FrameTreeNode is removed.
  void MarkRenderFrameDisposed();

  // Swap out the internal RFH when cross-origin navigation occurs.
  void UpdateRenderFrameHost(content::RenderFrameHost* rfh);

  const mojo::Remote<mojom::ElectronRenderer>& GetRendererApi();
  void MaybeSetupMojoConnection();
  void TeardownMojoConnection();
  void OnRendererConnectionError();

  [[nodiscard]] bool HasRenderFrame() const;

  // Throws a JS error if HasRenderFrame() is false.
  // WebFrameMain can outlive its RenderFrameHost pointer,
  // so we need to check whether its been disposed of
  // prior to accessing it.
  bool CheckRenderFrame() const;

  v8::Local<v8::Promise> ExecuteJavaScript(gin::Arguments* args,
                                           const std::u16string& code);
  void CopyVideoFrameAt(int x, int y);
  void SaveVideoFrameAs(int x, int y);
  bool Reload();
  bool IsDestroyed() const;
  void Send(v8::Isolate* isolate,
            bool internal,
            const std::string& channel,
            v8::Local<v8::Value> args);
  void PostMessage(v8::Isolate* isolate,
                   const std::string& channel,
                   v8::Local<v8::Value> message_value,
                   std::optional<v8::Local<v8::Value>> transfer);

  bool Detached() const;
  content::FrameTreeNodeId FrameTreeNodeID() const;
  std::string Name() const;
  std::string FrameToken() const;
  base::ProcessId OSProcessID() const;
  int32_t ProcessID() const;
  int RoutingID() const;
  GURL URL() const;
  std::string Origin() const;
  blink::mojom::PageVisibilityState VisibilityState() const;

  content::RenderFrameHost* Top() const;
  content::RenderFrameHost* Parent() const;
  std::vector<content::RenderFrameHost*> Frames() const;
  std::vector<content::RenderFrameHost*> FramesInSubtree() const;

  std::string_view LifecycleStateForTesting() const;

  v8::Local<v8::Promise> CollectDocumentJSCallStack(gin::Arguments* args);
  void CollectedJavaScriptCallStack(
      gin_helper::Promise<base::Value> promise,
      const std::string& untrusted_javascript_call_stack,
      const std::optional<blink::LocalFrameToken>& remote_frame_token);

  void DOMContentLoaded();

  GC_PLUGIN_IGNORE(
      "Context tracking of the renderer remote is not needed in the browser "
      "process.")
  mojo::Remote<mojom::ElectronRenderer> renderer_api_;
  GC_PLUGIN_IGNORE(
      "Context tracking of the pending receiver is not needed in the browser "
      "process.")
  mojo::PendingReceiver<mojom::ElectronRenderer> pending_receiver_;

  content::FrameTreeNodeId frame_tree_node_id_;
  content::GlobalRenderFrameHostToken frame_token_;

  // Whether the RenderFrameHost has been removed and that it should no longer
  // be accessed.
  bool render_frame_disposed_ = false;

  // Whether the content::RenderFrameHost is detached from the frame
  // tree. This can occur while it's running unload handlers.
  bool render_frame_detached_;

  // Roots this object in the cppgc heap for the lifetime of the underlying
  // frame so that registered IPC handlers keep dispatching even when no JS
  // reference remains.
  gin_helper::SelfKeepAlive<WebFrameMain> keep_alive_{this};

  gin::WeakCellFactory<WebFrameMain> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
