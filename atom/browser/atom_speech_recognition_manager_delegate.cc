// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_speech_recognition_manager_delegate.h"

#include <string>

#include "base/callback.h"

namespace atom {

AtomSpeechRecognitionManagerDelegate::AtomSpeechRecognitionManagerDelegate() {
}

AtomSpeechRecognitionManagerDelegate::~AtomSpeechRecognitionManagerDelegate() {
}

void AtomSpeechRecognitionManagerDelegate::OnRecognitionStart(int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnAudioStart(int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnEnvironmentEstimationComplete(
    int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnSoundStart(int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnSoundEnd(int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnAudioEnd(int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnRecognitionEnd(int session_id) {
}

void AtomSpeechRecognitionManagerDelegate::OnRecognitionResults(
    int session_id, const content::SpeechRecognitionResults& result) {
}

void AtomSpeechRecognitionManagerDelegate::OnRecognitionError(
    int session_id, const content::SpeechRecognitionError& error) {
}

void AtomSpeechRecognitionManagerDelegate::OnAudioLevelsChange(
    int session_id, float volume, float noise_volume) {
}

void AtomSpeechRecognitionManagerDelegate::CheckRecognitionIsAllowed(
    int session_id,
    base::Callback<void(bool ask_user, bool is_allowed)> callback) {
  callback.Run(true, true);
}

content::SpeechRecognitionEventListener*
AtomSpeechRecognitionManagerDelegate::GetEventListener() {
  return this;
}

bool AtomSpeechRecognitionManagerDelegate::FilterProfanities(
    int render_process_id) {
  return false;
}

}  // namespace atom
