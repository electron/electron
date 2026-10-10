// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_

#include <optional>

#include "shell/browser/api/electron_api_view.h"
#include "shell/common/color_util.h"
#include "v8/include/cppgc/member.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class Dictionary;
}  // namespace gin_helper

namespace electron {
class NativeWindow;
}  // namespace electron

namespace electron::api {

class WebContents;
class WebContentsViewHost;

class WebContentsView final : public View {
 public:
  static WebContentsView* New(gin::Arguments* args);

  // Creates a WebContentsView through its JavaScript constructor.
  static WebContentsView* Create(v8::Isolate* isolate,
                                 const gin_helper::Dictionary& web_preferences);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  // gin_helper::Constructible
  using ConstructibleParent = View;
  static void FillObjectTemplate(v8::Isolate* isolate,
                                 v8::Local<v8::ObjectTemplate> templ);
  static const char* GetClassName() { return "WebContentsView"; }

  // Make public for cppgc::MakeGarbageCollected.
  explicit WebContentsView(WebContents* web_contents);
  ~WebContentsView() override;

  // Public APIs.
  WebContents* GetWebContents();
  void SetBackgroundColor(std::optional<WrappedSkColor> color);
  void SetBorderRadius(int radius);
  void SetInteractive(bool interactive) override;

  // Lets |window| hit test this view's draggable regions until the view is
  // removed from it or its native peer is released.
  void RegisterDraggableRegionProvider(NativeWindow* window);

 private:
  friend class WebContentsViewHost;

  // View:
  bool IsUsable() const override;

  WebContentsViewHost* web_contents_view_host() const;
  WebContents* GetLiveWebContents() const;
  void OnWebContentsDestroyed();

  cppgc::Member<WebContents> api_web_contents_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_
