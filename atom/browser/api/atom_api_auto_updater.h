// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
#define ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "atom/browser/auto_updater_delegate.h"
#include "atom/common/api/atom_api_event_emitter.h"

namespace atom {

namespace api {

class AutoUpdater : public EventEmitter,
                    public auto_updater::AutoUpdaterDelegate {
 public:
  virtual ~AutoUpdater();

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit AutoUpdater(v8::Handle<v8::Object> wrapper);

  // AutoUpdaterDelegate implementations.
  virtual void OnError(const std::string& error) OVERRIDE;
  virtual void OnCheckingForUpdate() OVERRIDE;
  virtual void OnUpdateAvailable() OVERRIDE;
  virtual void OnUpdateNotAvailable() OVERRIDE;
  virtual void OnUpdateDownloaded(
      const std::string& release_notes,
      const std::string& release_name,
      const base::Time& release_date,
      const std::string& update_url,
      const base::Closure& quit_and_install) OVERRIDE;

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void SetFeedURL(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CheckForUpdates(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void ContinueUpdate(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void QuitAndInstall(const v8::FunctionCallbackInfo<v8::Value>& args);

  base::Closure quit_and_install_;

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
