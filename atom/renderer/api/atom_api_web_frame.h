// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_
#define ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_

#include <string>

#include "atom/renderer/guest_view_container.h"
#include "base/memory/scoped_ptr.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace blink {
class WebLocalFrame;
}

namespace mate {
class Arguments;
}

namespace atom {

namespace api {

class SpellCheckClient;

class WebFrame : public mate::Wrappable {
 public:
  static mate::Handle<WebFrame> Create(v8::Isolate* isolate);

 private:
  WebFrame();
  virtual ~WebFrame();

  void SetName(const std::string& name);

  double SetZoomLevel(double level);
  double GetZoomLevel() const;
  double SetZoomFactor(double factor);
  double GetZoomFactor() const;

  void SetZoomLevelLimits(double min_level, double max_level);

  v8::Local<v8::Value> RegisterEmbedderCustomElement(
      const base::string16& name, v8::Local<v8::Object> options);
  void RegisterElementResizeCallback(
      int element_instance_id,
      const GuestViewContainer::ResizeCallback& callback);
  void AttachGuest(int element_instance_id);

  // Set the provider that will be used by SpellCheckClient for spell check.
  void SetSpellCheckProvider(mate::Arguments* args,
                             const std::string& language,
                             bool auto_spell_correct_turned_on,
                             v8::Local<v8::Object> provider);

  void RegisterURLSchemeAsSecure(const std::string& scheme);
  void RegisterURLSchemeAsBypassingCsp(const std::string& scheme);
  void RegisterURLSchemeAsPrivileged(const std::string& scheme);

  // mate::Wrappable:
  virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

  scoped_ptr<SpellCheckClient> spell_check_client_;

  blink::WebLocalFrame* web_frame_;

  DISALLOW_COPY_AND_ASSIGN(WebFrame);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_
