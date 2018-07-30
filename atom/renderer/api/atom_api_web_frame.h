// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_
#define ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/renderer/guest_view_container.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"
#include "third_party/WebKit/public/platform/WebCache.h"

namespace blink {
class WebLocalFrame;
}

namespace mate {
class Dictionary;
class Arguments;
}  // namespace mate

namespace atom {

namespace api {

class SpellCheckClient;

class WebFrame : public mate::Wrappable<WebFrame> {
 public:
  static mate::Handle<WebFrame> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 private:
  explicit WebFrame(v8::Isolate* isolate);
  explicit WebFrame(v8::Isolate* isolate, blink::WebLocalFrame* blink_frame);
  ~WebFrame() override;

  void SetName(const std::string& name);

  double SetZoomLevel(double level);
  double GetZoomLevel() const;
  double SetZoomFactor(double factor);
  double GetZoomFactor() const;

  void SetVisualZoomLevelLimits(double min_level, double max_level);
  void SetLayoutZoomLevelLimits(double min_level, double max_level);

  v8::Local<v8::Value> RegisterEmbedderCustomElement(
      const base::string16& name,
      v8::Local<v8::Object> options);
  void RegisterElementResizeCallback(
      int element_instance_id,
      const GuestViewContainer::ResizeCallback& callback);
  int AttachIframeGuest(int element_instance_id,
                        v8::Local<v8::Value> content_window);
  void DetachGuest(int element_instance_id);

  // Set the provider that will be used by SpellCheckClient for spell check.
  void SetSpellCheckProvider(mate::Arguments* args,
                             const std::string& language,
                             bool auto_spell_correct_turned_on,
                             v8::Local<v8::Object> provider);

  void RegisterURLSchemeAsBypassingCSP(const std::string& scheme);
  void RegisterURLSchemeAsPrivileged(const std::string& scheme,
                                     mate::Arguments* args);

  // Editing.
  void InsertText(const std::string& text);
  void InsertCSS(const std::string& css);

  // Executing scripts.
  void ExecuteJavaScript(const base::string16& code, mate::Arguments* args);
  void ExecuteJavaScriptInIsolatedWorld(
      int world_id,
      const std::vector<mate::Dictionary>& scripts,
      mate::Arguments* args);

  // Isolated world related methods
  void SetIsolatedWorldSecurityOrigin(int world_id,
                                      const std::string& origin_url);
  void SetIsolatedWorldContentSecurityPolicy(
      int world_id,
      const std::string& security_policy);
  void SetIsolatedWorldHumanReadableName(int world_id, const std::string& name);

  // Resource related methods
  blink::WebCache::ResourceTypeStats GetResourceUsage(v8::Isolate* isolate);
  void ClearCache(v8::Isolate* isolate);

  // Frame navigation
  v8::Local<v8::Value> Opener() const;
  v8::Local<v8::Value> Parent() const;
  v8::Local<v8::Value> Top() const;
  v8::Local<v8::Value> FirstChild() const;
  v8::Local<v8::Value> NextSibling() const;
  v8::Local<v8::Value> GetFrameForSelector(const std::string& selector) const;
  v8::Local<v8::Value> FindFrameByName(const std::string& name) const;
  v8::Local<v8::Value> FindFrameByRoutingId(int routing_id) const;
  v8::Local<v8::Value> RoutingId() const;

  std::unique_ptr<SpellCheckClient> spell_check_client_;

  blink::WebLocalFrame* web_frame_;

  DISALLOW_COPY_AND_ASSIGN(WebFrame);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_WEB_FRAME_H_
