// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_JAVASCRIPT_BINDINGS_H_
#define ATOM_COMMON_JAVASCRIPT_BINDINGS_H_

#include "content/public/renderer/render_view_observer.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"

namespace mate {
class Arguments;
}

namespace atom {

class JavascriptBindings : public content::RenderViewObserver,
                           public extensions::ObjectBackedNativeHandler {
 public:
  explicit JavascriptBindings(content::RenderView* render_view,
                              extensions::ScriptContext* context);
  virtual ~JavascriptBindings();

  void GetBinding(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  base::string16 IPCSendSync(mate::Arguments* args,
                        const base::string16& channel,
                        const base::ListValue& arguments);
  void IPCSend(mate::Arguments* args,
                        const base::string16& channel,
                        const base::ListValue& arguments);
  v8::Local<v8::Value> GetHiddenValue(v8::Isolate* isolate,
                                    v8::Local<v8::String> key);
  void SetHiddenValue(v8::Isolate* isolate,
                      v8::Local<v8::String> key,
                      v8::Local<v8::Value> value);

  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnBrowserMessage(bool all_frames,
                        const base::string16& channel,
                        const base::ListValue& args);
  DISALLOW_COPY_AND_ASSIGN(JavascriptBindings);
};

}  // namespace atom

#endif  // ATOM_COMMON_JAVASCRIPT_BINDINGS_H_
