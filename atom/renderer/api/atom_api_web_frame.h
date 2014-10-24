// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_
#define ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_

#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace blink {
class WebLocalFrame;
}

namespace atom {

namespace api {

class WebFrame : public mate::Wrappable {
 public:
  static mate::Handle<WebFrame> Create(v8::Isolate* isolate);

 private:
  WebFrame();
  virtual ~WebFrame();

  double SetZoomLevel(double level);
  double GetZoomLevel() const;
  double SetZoomFactor(double factor);
  double GetZoomFactor() const;

  v8::Handle<v8::Value> RegisterEmbedderCustomElement(
      const base::string16& name, v8::Handle<v8::Object> options);

  // mate::Wrappable:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

  blink::WebLocalFrame* web_frame_;

  DISALLOW_COPY_AND_ASSIGN(WebFrame);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_
