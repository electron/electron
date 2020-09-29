// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_

#include <memory>
#include <string>
#include <vector>

#include "gin/handle.h"
#include "gin/wrappable.h"

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
class WebFrame : public gin::Wrappable<WebFrame> {
 public:
  static gin::Handle<WebFrame> FromID(v8::Isolate* isolate,
                                      int render_process_id,
                                      int render_frame_id);
  static gin::Handle<WebFrame> From(
      v8::Isolate* isolate,
      content::RenderFrameHost* render_frame_host);

  // Called to mark any RenderFrameHost as disposed by any WebFrame that may
  // be holding a weak reference.
  static void RenderFrameDeleted(content::RenderFrameHost* rfh);

  // Mark RenderFrameHost as disposed and to no longer access it. This can
  // occur upon frame navigation.
  void MarkRenderFrameDisposed();

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  explicit WebFrame(content::RenderFrameHost* render_frame);
  ~WebFrame() override;

 private:
  // WebFrame can outlive its RenderFrameHost pointer so we need to check
  // whether its been disposed of prior to accessing it.
  bool CheckRenderFrame() const;

  v8::Local<v8::Promise> ExecuteJavaScript(const base::string16& code,
                                           bool has_user_gesture,
                                           gin::Arguments* args);
  bool Reload(v8::Isolate* isolate);

  int FrameTreeNodeID(v8::Isolate* isolate) const;
  int ProcessID(v8::Isolate* isolate) const;
  int RoutingID(v8::Isolate* isolate) const;
  GURL URL(v8::Isolate* isolate) const;

  content::RenderFrameHost* Top(v8::Isolate* isolate) const;
  content::RenderFrameHost* Parent(v8::Isolate* isolate) const;
  std::vector<content::RenderFrameHost*> Frames(v8::Isolate* isolate) const;
  std::vector<content::RenderFrameHost*> FramesInSubtree(
      v8::Isolate* isolate) const;

  content::RenderFrameHost* render_frame_ = nullptr;

  // Whether the RenderFrameHost has been removed and that it should no longer
  // be accessed.
  bool render_frame_disposed_ = false;

  DISALLOW_COPY_AND_ASSIGN(WebFrame);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
