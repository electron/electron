// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/browser_observer.h"
#include "native_mate/handle.h"

namespace base {
class FilePath;
}

namespace mate {
class Arguments;
}

namespace atom {

namespace api {

class App : public mate::EventEmitter,
            public BrowserObserver {
 public:
  static mate::Handle<App> Create(v8::Isolate* isolate);

 protected:
  App();
  virtual ~App();

  // BrowserObserver:
  void OnBeforeQuit(bool* prevent_default) override;
  void OnWillQuit(bool* prevent_default) override;
  void OnWindowAllClosed() override;
  void OnQuit() override;
  void OnOpenFile(bool* prevent_default, const std::string& file_path) override;
  void OnOpenURL(const std::string& url) override;
  void OnActivateWithNoOpenWindows() override;
  void OnWillFinishLaunching() override;
  void OnFinishLaunching() override;

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  // Get/Set the pre-defined path in PathService.
  base::FilePath GetPath(mate::Arguments* args, const std::string& name);
  void SetPath(mate::Arguments* args,
               const std::string& name,
               const base::FilePath& path);

  void SetDesktopName(const std::string& desktop_name);
  void SetAppUserModelId(const std::string& app_id);
  v8::Local<v8::Value> DefaultSession(v8::Isolate* isolate);

  v8::Global<v8::Value> default_session_;

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
