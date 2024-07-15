// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_speech_recognition_manager_delegate.h"

#include <utility>

#include "base/functional/callback.h"

namespace electron {

ElectronSpeechRecognitionManagerDelegate::
    ElectronSpeechRecognitionManagerDelegate() = default;

ElectronSpeechRecognitionManagerDelegate::
    ~ElectronSpeechRecognitionManagerDelegate() = default;

void ElectronSpeechRecognitionManagerDelegate::CheckRecognitionIsAllowed(
    int session_id,
    base::OnceCallback<void(bool ask_user, bool is_allowed)> callback) {
  std::move(callback).Run(true, true);
}

content::SpeechRecognitionEventListener*
ElectronSpeechRecognitionManagerDelegate::GetEventListener() {
  return nullptr;
}

void ElectronSpeechRecognitionManagerDelegate::BindSpeechRecognitionContext(
    mojo::PendingReceiver<media::mojom::SpeechRecognitionContext> receiver) {}

}  // namespace electron
