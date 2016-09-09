// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/renderer/extensions/content_settings_bindings.h"

#include <string>
#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/renderer/content_settings_manager.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/renderer/script_context.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebDocument.h"


namespace brave {

ContentSettingsBindings::ContentSettingsBindings(
    extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {
  RouteFunction(
      "getCurrent",
      base::Bind(&ContentSettingsBindings::GetCurrentSetting,
          base::Unretained(this)));
  RouteFunction(
      "getContentTypes",
      base::Bind(&ContentSettingsBindings::GetContentTypes,
          base::Unretained(this)));
}

ContentSettingsBindings::~ContentSettingsBindings() {
}

void ContentSettingsBindings::GetCurrentSetting(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  const std::string content_type =
      mate::V8ToString(args[0].As<v8::String>());
  bool incognito = args[1].As<v8::Boolean>()->Value();

  auto render_view = context()->GetRenderFrame()->GetRenderView();
  GURL main_frame_url =
      render_view->GetWebView()->mainFrame()->document().url();

  ContentSetting setting =
    atom::ContentSettingsManager::GetInstance()->GetSetting(
          main_frame_url,
          context()->web_frame()->document().url(),
          content_type,
          incognito);


  args.GetReturnValue().Set(
    mate::Converter<ContentSetting>::ToV8(context()->isolate(), setting));
}

void ContentSettingsBindings::GetContentTypes(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::vector<std::string> content_types =
    atom::ContentSettingsManager::GetInstance()->GetContentTypes();

  args.GetReturnValue().Set(
      mate::Converter<std::vector<std::string>>::ToV8(
          context()->isolate(), content_types));
}

}  // namespace brave
