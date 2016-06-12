// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "base/values.h"
#include "content/public/renderer/render_process_observer.h"

namespace atom {

class PreferencesManager : public content::RenderProcessObserver {
 public:
  PreferencesManager();
  ~PreferencesManager() override;

  const base::ListValue* preferences() const { return preferences_.get(); }

 private:
  // content::RenderThreadObserver:
  bool OnControlMessageReceived(const IPC::Message& message) override;

  void OnUpdatePreferences(const base::ListValue& preferences);

  std::unique_ptr<base::ListValue> preferences_;

  DISALLOW_COPY_AND_ASSIGN(PreferencesManager);
};

}  // namespace atom
