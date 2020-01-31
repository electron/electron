// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
#define SHELL_BROWSER_API_ATOM_API_AUTO_UPDATER_H_

#include <string>

#include "gin/handle.h"
#include "shell/browser/auto_updater.h"
#include "shell/browser/window_list_observer.h"
#include "shell/common/gin_helper/event_emitter.h"

namespace electron {

namespace api {

class AutoUpdater : public gin_helper::EventEmitter<AutoUpdater>,
                    public auto_updater::Delegate,
                    public WindowListObserver {
 public:
  static gin::Handle<AutoUpdater> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit AutoUpdater(v8::Isolate* isolate);
  ~AutoUpdater() override;

  // Delegate implementations.
  void OnError(const std::string& error) override;
  void OnError(const std::string& message,
               const int code,
               const std::string& domain) override;
  void OnCheckingForUpdate() override;
  void OnUpdateAvailable() override;
  void OnUpdateNotAvailable() override;
  void OnUpdateDownloaded(const std::string& release_notes,
                          const std::string& release_name,
                          const base::Time& release_date,
                          const std::string& update_url) override;

  // WindowListObserver:
  void OnWindowAllClosed() override;

 private:
  std::string GetFeedURL();
  void SetFeedURL(gin_helper::Arguments* args);
  void QuitAndInstall();

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
