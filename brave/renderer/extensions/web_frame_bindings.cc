// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/renderer/extensions/web_frame_bindings.h"

#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "extensions/renderer/console.h"
#include "extensions/renderer/script_context.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptExecutionCallback.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"

namespace brave {

namespace {

class ScriptExecutionCallback : public blink::WebScriptExecutionCallback {
 public:
  ScriptExecutionCallback() {}
  ~ScriptExecutionCallback() override {}

  void completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScriptExecutionCallback);
};

}  // namespace

WebFrameBindings::WebFrameBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {
  RouteFunction(
      "executeJavaScript",
      base::Bind(&WebFrameBindings::ExecuteJavaScript,
          base::Unretained(this)));
  RouteFunction(
      "setSpellCheckProvider",
      base::Bind(&WebFrameBindings::SetSpellCheckProvider,
          base::Unretained(this)));
  RouteFunction(
      "setGlobal",
      base::Bind(&WebFrameBindings::SetGlobal, base::Unretained(this)));
}

WebFrameBindings::~WebFrameBindings() {
}

void WebFrameBindings::Invalidate() {
  // only remove the spell check client when the main frame is invalidated
  if (!context()->web_frame()->parent()) {
    context()->web_frame()->view()->setSpellCheckClient(nullptr);
    spell_check_client_.reset(nullptr);
  }
  ObjectBackedNativeHandler::Invalidate();
}

void WebFrameBindings::SetSpellCheckProvider(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (context()->web_frame()->parent()) {
    extensions::console::Warn(context()->GetRenderFrame(),
      "spellcheck provider can only be set by the main frame");
    return;
  }

  const std::string lang = mate::V8ToString(args[0].As<v8::String>());
  bool auto_spell_correct_turned_on = args[1].As<v8::Boolean>()->Value();
  v8::Local<v8::Object> provider = v8::Local<v8::Object>::Cast(args[2]);

  std::unique_ptr<atom::api::SpellCheckClient> spell_check_client(
      new atom::api::SpellCheckClient(
          lang, auto_spell_correct_turned_on, GetIsolate(), provider));
  context()->web_frame()->view()->setSpellCheckClient(spell_check_client.get());
  spell_check_client_.swap(spell_check_client);
}

void WebFrameBindings::ExecuteJavaScript(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  const base::string16 code = base::UTF8ToUTF16(mate::V8ToString(args[0]));
  v8::Local<v8::Context> main_context =
      context()->web_frame()->mainWorldScriptContext();
  v8::Context::Scope context_scope(main_context);

  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback());
  context()->web_frame()->requestExecuteScriptAndReturnValue(
      blink::WebScriptSource(code),
      true,
      callback.release());
}


void WebFrameBindings::SetGlobal(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = GetIsolate();
  v8::Local<v8::Array> array(v8::Local<v8::Array>::Cast(args[0]));
  std::vector<v8::Handle<v8::String>> path;
  mate::Converter<std::vector<v8::Handle<v8::String>>>::FromV8(
      isolate, array, &path);
  v8::Local<v8::Object> value = v8::Local<v8::Object>::Cast(args[1]);

  v8::Local<v8::Context> main_context =
      context()->web_frame()->mainWorldScriptContext();
  v8::Context::Scope context_scope(main_context);

  if (!ContextCanAccessObject(main_context, main_context->Global(), false)) {
    extensions::console::Warn(context()->GetRenderFrame(),
      "cannot access global in main frame script context");
    return;
  }

  v8::Handle<v8::Object> obj;
  for (std::vector<v8::Handle<v8::String>>::const_iterator iter =
             path.begin();
         iter != path.end();
         ++iter) {
    if (iter == path.begin()) {
      obj = v8::Handle<v8::Object>::Cast(main_context->Global()->Get(*iter));
    } else if (iter == path.end()-1) {
      obj->Set(*iter, value);
    } else {
      obj = v8::Handle<v8::Object>::Cast(obj->Get(*iter));
    }
  }
}

}  // namespace brave
