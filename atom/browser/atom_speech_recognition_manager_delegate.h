// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
#define ATOM_BROWSER_ATOM_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/speech_recognition_event_listener.h"
#include "content/public/browser/speech_recognition_manager_delegate.h"

namespace atom {

class AtomSpeechRecognitionManagerDelegate
    : public content::SpeechRecognitionManagerDelegate,
      public content::SpeechRecognitionEventListener {
 public:
  AtomSpeechRecognitionManagerDelegate();
  virtual ~AtomSpeechRecognitionManagerDelegate();

  // content::SpeechRecognitionEventListener:
  void OnRecognitionStart(int session_id) override;
  void OnAudioStart(int session_id) override;
  void OnEnvironmentEstimationComplete(int session_id) override;
  void OnSoundStart(int session_id) override;
  void OnSoundEnd(int session_id) override;
  void OnAudioEnd(int session_id) override;
  void OnRecognitionEnd(int session_id) override;
  void OnRecognitionResults(
      int session_id, const content::SpeechRecognitionResults& result) override;
  void OnRecognitionError(
      int session_id, const content::SpeechRecognitionError& error) override;
  void OnAudioLevelsChange(int session_id, float volume,
                           float noise_volume) override;

  // content::SpeechRecognitionManagerDelegate:
  void GetDiagnosticInformation(bool* can_report_metrics,
                                std::string* hardware_info) override;
  void CheckRecognitionIsAllowed(
      int session_id,
      base::Callback<void(bool ask_user, bool is_allowed)> callback) override;
  content::SpeechRecognitionEventListener* GetEventListener() override;
  bool FilterProfanities(int render_process_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomSpeechRecognitionManagerDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
