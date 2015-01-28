// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
#define ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_

#include <string>

#include "base/callback.h"
#include "atom/browser/api/event_emitter.h"
#include "atom/browser/auto_updater_delegate.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class AutoUpdater : public mate::EventEmitter,
                    public auto_updater::AutoUpdaterDelegate {
 public:
  static mate::Handle<AutoUpdater> Create(v8::Isolate* isolate);

 protected:
  AutoUpdater();
  virtual ~AutoUpdater();

  // AutoUpdaterDelegate implementations.
  void OnError(const std::string& error) override;
  void OnCheckingForUpdate() override;
  void OnUpdateAvailable() override;
  void OnUpdateNotAvailable() override;
  void OnUpdateDownloaded(
      const std::string& release_notes,
      const std::string& release_name,
      const base::Time& release_date,
      const std::string& update_url,
      const base::Closure& quit_and_install) override;

  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  void QuitAndInstall();

  base::Closure quit_and_install_;

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
