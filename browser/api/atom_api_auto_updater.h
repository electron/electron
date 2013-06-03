// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
#define ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_

#include "browser/api/atom_api_event_emitter.h"
#include "browser/auto_updater_delegate.h"

namespace atom {

namespace api {

class AutoUpdater : public EventEmitter,
                    public auto_updater::AutoUpdaterDelegate {
 public:
  virtual ~AutoUpdater();

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit AutoUpdater(v8::Handle<v8::Object> wrapper);

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTO_UPDATER_H_
