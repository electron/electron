// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_TTS_CONTROLLER_IMPL_H_
#define CHROME_BROWSER_SPEECH_TTS_CONTROLLER_IMPL_H_

#include <queue>
#include <set>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/speech/tts_controller.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

// Singleton class that manages text-to-speech for the TTS and TTS engine
// extension APIs, maintaining a queue of pending utterances and keeping
// track of all state.
class TtsControllerImpl : public TtsController {
 public:
  // Get the single instance of this class.
  static TtsControllerImpl* GetInstance();

  // TtsController methods
  virtual bool IsSpeaking() override;
  virtual void SpeakOrEnqueue(Utterance* utterance) override;
  virtual void Stop() override;
  virtual void Pause() override;
  virtual void Resume() override;
  virtual void OnTtsEvent(int utterance_id,
                  TtsEventType event_type,
                  int char_index,
                  const std::string& error_message) override;
  virtual void GetVoices(content::BrowserContext* browser_context,
                         std::vector<VoiceData>* out_voices) override;
  virtual void VoicesChanged() override;
  virtual void AddVoicesChangedDelegate(
      VoicesChangedDelegate* delegate) override;
  virtual void RemoveVoicesChangedDelegate(
      VoicesChangedDelegate* delegate) override;
  virtual void SetTtsEngineDelegate(TtsEngineDelegate* delegate) override;
  virtual TtsEngineDelegate* GetTtsEngineDelegate() override;
  virtual void SetPlatformImpl(TtsPlatformImpl* platform_impl) override;
  virtual int QueueSize() override;

 protected:
  TtsControllerImpl();
  virtual ~TtsControllerImpl();

 private:
  // Get the platform TTS implementation (or injected mock).
  TtsPlatformImpl* GetPlatformImpl();

  // Start speaking the given utterance. Will either take ownership of
  // |utterance| or delete it if there's an error. Returns true on success.
  void SpeakNow(Utterance* utterance);

  // Clear the utterance queue. If send_events is true, will send
  // TTS_EVENT_CANCELLED events on each one.
  void ClearUtteranceQueue(bool send_events);

  // Finalize and delete the current utterance.
  void FinishCurrentUtterance();

  // Start speaking the next utterance in the queue.
  void SpeakNextUtterance();

  // Given an utterance and a vector of voices, return the
  // index of the voice that best matches the utterance.
  int GetMatchingVoice(const Utterance* utterance,
                       std::vector<VoiceData>& voices);

  friend struct DefaultSingletonTraits<TtsControllerImpl>;

  // The current utterance being spoken.
  Utterance* current_utterance_;

  // Whether the queue is paused or not.
  bool paused_;

  // A queue of utterances to speak after the current one finishes.
  std::queue<Utterance*> utterance_queue_;

  // A set of delegates that want to be notified when the voices change.
  std::set<VoicesChangedDelegate*> voices_changed_delegates_;

  // A pointer to the platform implementation of text-to-speech, for
  // dependency injection.
  TtsPlatformImpl* platform_impl_;

  // The delegate that processes TTS requests with user-installed extensions.
  TtsEngineDelegate* tts_engine_delegate_;

  DISALLOW_COPY_AND_ASSIGN(TtsControllerImpl);
};

#endif  // CHROME_BROWSER_SPEECH_TTS_CONTROLLER_IMPL_H_