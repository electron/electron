// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_WEB_VIEW_H_
#define ATOM_RENDERER_API_ATOM_API_WEB_VIEW_H_

#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace WebKit {
class WebView;
}

namespace atom {

namespace api {

class WebView : public mate::Wrappable {
 public:
  static mate::Handle<WebView> Create(v8::Isolate* isolate);

 private:
  WebView();
  virtual ~WebView();

  void SetZoomLevel(double level);
  double GetZoomLevel() const;

  // mate::Wrappable:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

  WebKit::WebView* web_view_;

  DISALLOW_COPY_AND_ASSIGN(WebView);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_WEB_VIEW_H_
