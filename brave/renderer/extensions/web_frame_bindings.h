// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_RENDERER_EXTENSIONS_WEB_FRAME_BINDINGS_H_
#define BRAVE_RENDERER_EXTENSIONS_WEB_FRAME_BINDINGS_H_

#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"
#include "atom/renderer/api/atom_api_spell_check_client.h"

namespace brave {

class WebFrameBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit WebFrameBindings(extensions::ScriptContext* context);
  virtual ~WebFrameBindings();

  void WebFrame(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetSpellCheckProvider(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetGlobal(const v8::FunctionCallbackInfo<v8::Value>& args);
  void ExecuteJavaScript(const v8::FunctionCallbackInfo<v8::Value>& args);
  void Invalidate() override;

 private:
  void BindToGC(const v8::FunctionCallbackInfo<v8::Value>& args);

  std::unique_ptr<atom::api::SpellCheckClient> spell_check_client_;

  DISALLOW_COPY_AND_ASSIGN(WebFrameBindings);
};

}  // namespace atom

#endif  // BRAVE_RENDERER_EXTENSIONS_WEB_FRAME_BINDINGS_H_
