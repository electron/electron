// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
#define ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "browser/auto_updater_delegate.h"
#include "common/api/atom_api_event_emitter.h"

namespace atom {

namespace api {

class AutoUpdater : public EventEmitter,
                    public auto_updater::AutoUpdaterDelegate {
 public:
  virtual ~AutoUpdater();

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit AutoUpdater(v8::Handle<v8::Object> wrapper);

  virtual void WillInstallUpdate(const std::string& version,
                                 const base::Closure& install) OVERRIDE;
  virtual void ReadyForUpdateOnQuit(
      const std::string& version,
      const base::Closure& quit_and_install) OVERRIDE;

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void SetFeedURL(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetAutomaticallyChecksForUpdates(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SetAutomaticallyDownloadsUpdates(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CheckForUpdates(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void CheckForUpdatesInBackground(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  static void ContinueUpdate(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void QuitAndInstall(const v8::FunctionCallbackInfo<v8::Value>& args);

  base::Closure continue_update_;
  base::Closure quit_and_install_;

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
