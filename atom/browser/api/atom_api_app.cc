// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_app.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "atom/browser/browser.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

#if defined(OS_LINUX)
#include "base/nix/xdg_util.h"
#endif

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

std::string App::GetDataPath() {
  base::FilePath path;
#if defined(OS_LINUX)
  scoped_ptr<base::Environment> env(base::Environment::Create());
  path = base::nix::GetXDGDirectory(env.get(),
                                    base::nix::kXdgConfigHomeEnvVar,
                                    base::nix::kDotConfigDir);
#else
  CHECK(PathService::Get(base::DIR_APP_DATA, &path));
#endif

  base::FilePath data_path = path.Append(
    base::FilePath::FromUTF8Unsafe(Browser::Get()->GetName()));

  // FilePath.value() returns a std::wstring in windows and
  // std::string on other platforms.
  std::vector<char> writable(data_path.value().begin(),
    data_path.value().end());
  writable.push_back('\0');

  return &writable[0];
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
                                       base::Unretained(browser)))
      .SetMethod("getDataPath", &App::GetDataPath);
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

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
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
  dict.SetMethod("dockHide",
                 base::Bind(&Browser::DockHide, base::Unretained(browser)));
  dict.SetMethod("dockShow",
                 base::Bind(&Browser::DockShow, base::Unretained(browser)));
#endif
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_app, Initialize)
