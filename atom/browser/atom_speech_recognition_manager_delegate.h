// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
#define ATOM_BROWSER_ATOM_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_

#include <string>

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
  virtual void OnRecognitionStart(int session_id) override;
  virtual void OnAudioStart(int session_id) override;
  virtual void OnEnvironmentEstimationComplete(int session_id) override;
  virtual void OnSoundStart(int session_id) override;
  virtual void OnSoundEnd(int session_id) override;
  virtual void OnAudioEnd(int session_id) override;
  virtual void OnRecognitionEnd(int session_id) override;
  virtual void OnRecognitionResults(
      int session_id, const content::SpeechRecognitionResults& result) override;
  virtual void OnRecognitionError(
      int session_id, const content::SpeechRecognitionError& error) override;
  virtual void OnAudioLevelsChange(int session_id, float volume,
                                   float noise_volume) override;

  // content::SpeechRecognitionManagerDelegate:
  virtual void GetDiagnosticInformation(bool* can_report_metrics,
                                        std::string* hardware_info) override;
  virtual void CheckRecognitionIsAllowed(
      int session_id,
      base::Callback<void(bool ask_user, bool is_allowed)> callback) override;
  virtual content::SpeechRecognitionEventListener* GetEventListener() override;
  virtual bool FilterProfanities(int render_process_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomSpeechRecognitionManagerDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
