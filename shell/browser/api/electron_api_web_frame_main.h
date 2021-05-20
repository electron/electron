// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/pinnable.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-forward.h"

class GURL;

namespace content {
class RenderFrameHost;
}

namespace gin {
class Arguments;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

namespace api {

// Bindings for accessing frames from the main process.
class WebFrameMain : public gin::Wrappable<WebFrameMain>,
                     public gin_helper::Pinnable<WebFrameMain>,
                     public gin_helper::Constructible<WebFrameMain> {
 public:
  // Create a new WebFrameMain and return the V8 wrapper of it.
  static gin::Handle<WebFrameMain> New(v8::Isolate* isolate);

  static gin::Handle<WebFrameMain> FromID(v8::Isolate* isolate,
                                          int render_process_id,
                                          int render_frame_id);
  static gin::Handle<WebFrameMain> From(
      v8::Isolate* isolate,
      content::RenderFrameHost* render_frame_host);

  // Called to mark any RenderFrameHost as disposed by any WebFrameMain that
  // may be holding a weak reference.
  static void RenderFrameDeleted(content::RenderFrameHost* rfh);
  static void RenderFrameCreated(content::RenderFrameHost* rfh);

  // Mark RenderFrameHost as disposed and to no longer access it. This can
  // occur upon frame navigation.
  void MarkRenderFrameDisposed();

  const mojo::Remote<mojom::ElectronRenderer>& GetRendererApi();

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);
  const char* GetTypeName() override;

 protected:
  explicit WebFrameMain(content::RenderFrameHost* render_frame);
  ~WebFrameMain() override;

 private:
  // WebFrameMain can outlive its RenderFrameHost pointer so we need to check
  // whether its been disposed of prior to accessing it.
  bool CheckRenderFrame() const;
  void Connect();

  v8::Local<v8::Promise> ExecuteJavaScript(gin::Arguments* args,
                                           const std::u16string& code);
  bool Reload();
  void Send(v8::Isolate* isolate,
            bool internal,
            const std::string& channel,
            v8::Local<v8::Value> args);
  void PostMessage(v8::Isolate* isolate,
                   const std::string& channel,
                   v8::Local<v8::Value> message_value,
                   absl::optional<v8::Local<v8::Value>> transfer);

  int FrameTreeNodeID() const;
  std::string Name() const;
  base::ProcessId OSProcessID() const;
  int ProcessID() const;
  int RoutingID() const;
  GURL URL() const;
  blink::mojom::PageVisibilityState VisibilityState() const;

  content::RenderFrameHost* Top() const;
  content::RenderFrameHost* Parent() const;
  std::vector<content::RenderFrameHost*> Frames() const;
  std::vector<content::RenderFrameHost*> FramesInSubtree() const;

  void OnRendererConnectionError();

  mojo::Remote<mojom::ElectronRenderer> renderer_api_;
  mojo::PendingReceiver<mojom::ElectronRenderer> pending_receiver_;

  content::RenderFrameHost* render_frame_ = nullptr;

  // Whether the RenderFrameHost has been removed and that it should no longer
  // be accessed.
  bool render_frame_disposed_ = false;

  base::WeakPtrFactory<WebFrameMain> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WebFrameMain);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
