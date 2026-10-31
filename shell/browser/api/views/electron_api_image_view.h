// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_IMAGE_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_IMAGE_VIEW_H_

#include "shell/browser/api/electron_api_view.h"

namespace gfx {
class Image;
}

namespace gin {
class Arguments;
}  // namespace gin

namespace views {
class ImageView;
}  // namespace views

namespace electron::api {

class ImageView final : public View {
 public:
  static ImageView* New(gin::Arguments* args);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  // gin_helper::Constructible
  using ConstructibleParent = View;
  static void FillObjectTemplate(v8::Isolate* isolate,
                                 v8::Local<v8::ObjectTemplate> templ);
  static const char* GetClassName() { return "ImageView"; }

  // Make public for cppgc::MakeGarbageCollected.
  ImageView();
  ~ImageView() override;

  void SetImage(const gfx::Image& image);

 private:
  views::ImageView* image_view() const;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_IMAGE_VIEW_H_
