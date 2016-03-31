// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/electron_speech_recognition_manager_delegate.h"

#include <string>

#include "base/callback.h"

namespace electron {

ElectronSpeechRecognitionManagerDelegate::ElectronSpeechRecognitionManagerDelegate() {
}

ElectronSpeechRecognitionManagerDelegate::~ElectronSpeechRecognitionManagerDelegate() {
}

void ElectronSpeechRecognitionManagerDelegate::OnRecognitionStart(int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnAudioStart(int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnEnvironmentEstimationComplete(
    int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnSoundStart(int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnSoundEnd(int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnAudioEnd(int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnRecognitionEnd(int session_id) {
}

void ElectronSpeechRecognitionManagerDelegate::OnRecognitionResults(
    int session_id, const content::SpeechRecognitionResults& result) {
}

void ElectronSpeechRecognitionManagerDelegate::OnRecognitionError(
    int session_id, const content::SpeechRecognitionError& error) {
}

void ElectronSpeechRecognitionManagerDelegate::OnAudioLevelsChange(
    int session_id, float volume, float noise_volume) {
}

void ElectronSpeechRecognitionManagerDelegate::GetDiagnosticInformation(
    bool* can_report_metrics, std::string* hardware_info) {
  *can_report_metrics = false;
}

void ElectronSpeechRecognitionManagerDelegate::CheckRecognitionIsAllowed(
    int session_id,
    base::Callback<void(bool ask_user, bool is_allowed)> callback) {
  callback.Run(true, true);
}

content::SpeechRecognitionEventListener*
ElectronSpeechRecognitionManagerDelegate::GetEventListener() {
  return this;
}

bool ElectronSpeechRecognitionManagerDelegate::FilterProfanities(
    int render_process_id) {
  return false;
}

}  // namespace electron
