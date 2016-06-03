// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/preferences_manager.h"

#include "atom/common/api/api_messages.h"
#include "content/public/renderer/render_thread.h"

namespace atom {

PreferencesManager::PreferencesManager() {
  content::RenderThread::Get()->AddObserver(this);
}

PreferencesManager::~PreferencesManager() {
}

bool PreferencesManager::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PreferencesManager, message)
    IPC_MESSAGE_HANDLER(AtomMsg_UpdatePreferences, OnUpdatePreferences)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PreferencesManager::OnUpdatePreferences(
    const base::ListValue& preferences) {
  preferences_ = preferences.CreateDeepCopy();
}

}  // namespace atom
