// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_app.h"

#include <string>

#include "base/values.h"
#include "base/command_line.h"
#include "atom/browser/browser.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

using atom::Browser;

namespace atom {

namespace api {

App::App() {
  Browser::Get()->AddObserver(this);
}

App::~App() {
  Browser::Get()->RemoveObserver(this);
}

void App::OnWillQuit(bool* prevent_default) {
  *prevent_default = Emit("will-quit");
}

void App::OnWindowAllClosed() {
  Emit("window-all-closed");
}

void App::OnOpenFile(bool* prevent_default, const std::string& file_path) {
  base::ListValue args;
  args.AppendString(file_path);
  *prevent_default = Emit("open-file", args);
}

void App::OnOpenURL(const std::string& url) {
  base::ListValue args;
  args.AppendString(url);
  Emit("open-url", args);
}

void App::OnActivateWithNoOpenWindows() {
  Emit("activate-with-no-open-windows");
}

void App::OnWillFinishLaunching() {
  Emit("will-finish-launching");
}

void App::OnFinishLaunching() {
  Emit("ready");
}

mate::ObjectTemplateBuilder App::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  Browser* browser = Browser::Get();
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("quit", base::Bind(&Browser::Quit,
                                    base::Unretained(browser)))
      .SetMethod("focus", base::Bind(&Browser::Focus,
                                     base::Unretained(browser)))
      .SetMethod("getVersion", base::Bind(&Browser::GetVersion,
                                          base::Unretained(browser)))
      .SetMethod("setVersion", base::Bind(&Browser::SetVersion,
                                          base::Unretained(browser)))
      .SetMethod("getName", base::Bind(&Browser::GetName,
                                       base::Unretained(browser)))
      .SetMethod("setName", base::Bind(&Browser::SetName,
                                       base::Unretained(browser)));
}

// static
mate::Handle<App> App::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new App);
}

}  // namespace api

}  // namespace atom


namespace {

void AppendSwitch(const std::string& switch_string, mate::Arguments* args) {
  std::string value;
  if (args->GetNext(&value))
    CommandLine::ForCurrentProcess()->AppendSwitchASCII(switch_string, value);
  else
    CommandLine::ForCurrentProcess()->AppendSwitch(switch_string);
}

#if defined(OS_MACOSX)
int DockBounce(const std::string& type) {
  int request_id = -1;
  if (type == "critical")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_CRITICAL);
  else if (type == "informational")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_INFORMATIONAL);
  return request_id;
}
#endif

void Initialize(v8::Handle<v8::Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  Browser* browser = Browser::Get();
  CommandLine* command_line = CommandLine::ForCurrentProcess();

  mate::Dictionary dict(isolate, exports);
  dict.Set("app", atom::api::App::Create(isolate));
  dict.SetMethod("appendSwitch", &AppendSwitch);
  dict.SetMethod("appendArgument",
                 base::Bind(&CommandLine::AppendArg,
                            base::Unretained(command_line)));
#if defined(OS_MACOSX)
  dict.SetMethod("dockBounce", &DockBounce);
  dict.SetMethod("dockCancelBounce",
                 base::Bind(&Browser::DockCancelBounce,
                            base::Unretained(browser)));
  dict.SetMethod("dockSetBadgeText",
                 base::Bind(&Browser::DockSetBadgeText,
                            base::Unretained(browser)));
  dict.SetMethod("dockGetBadgeText",
                 base::Bind(&Browser::DockGetBadgeText,
                            base::Unretained(browser)));
#endif
}

}  // namespace

NODE_MODULE(atom_browser_app, Initialize)
