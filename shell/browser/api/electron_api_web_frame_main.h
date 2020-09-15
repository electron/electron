// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_

#include <memory>
#include <string>

#include "content/public/browser/render_frame_host.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace gin_helper {
class Dictionary;
}

namespace electron {

namespace api {

class WebFrame : public gin::Wrappable<WebFrame> {
 public:
  static gin::Handle<WebFrame> FromID(v8::Isolate* isolate,
                                      int render_process_id,
                                      int render_frame_id);
  static gin::Handle<WebFrame> From(
      v8::Isolate* isolate,
      content::RenderFrameHost* render_frame_host);

  content::RenderFrameHost* render_frame() const { return render_frame_; }

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  WebFrame(content::RenderFrameHost* render_frame);
  ~WebFrame() override;

 private:
  bool Reload();
  int GetFrameTreeNodeID() const;
  int GetRoutingID() const;
  GURL GetURL() const;
  content::RenderFrameHost* GetParent();

  content::RenderFrameHost* render_frame_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WebFrame);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
