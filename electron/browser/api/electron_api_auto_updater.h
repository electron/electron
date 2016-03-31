// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_BROWSER_API_ELECTRON_API_AUTO_UPDATER_H_
#define ELECTRON_BROWSER_API_ELECTRON_API_AUTO_UPDATER_H_

#include <string>

#include "electron/browser/api/event_emitter.h"
#include "electron/browser/auto_updater.h"
#include "electron/browser/window_list_observer.h"
#include "native_mate/handle.h"

namespace electron {

namespace api {

class AutoUpdater : public mate::EventEmitter,
                    public auto_updater::Delegate,
                    public WindowListObserver {
 public:
  static mate::Handle<AutoUpdater> Create(v8::Isolate* isolate);

 protected:
  AutoUpdater();
  virtual ~AutoUpdater();

  // Delegate implementations.
  void OnError(const std::string& error) override;
  void OnCheckingForUpdate() override;
  void OnUpdateAvailable() override;
  void OnUpdateNotAvailable() override;
  void OnUpdateDownloaded(const std::string& release_notes,
                          const std::string& release_name,
                          const base::Time& release_date,
                          const std::string& update_url) override;

  // WindowListObserver:
  void OnWindowAllClosed() override;

  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  void QuitAndInstall();

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_BROWSER_API_ELECTRON_API_AUTO_UPDATER_H_
