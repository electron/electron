// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_TTS_PLATFORM_H_
#define CHROME_BROWSER_SPEECH_TTS_PLATFORM_H_

#include <string>

#include "chrome/browser/speech/tts_controller.h"

// Abstract class that defines the native platform TTS interface,
// subclassed by specific implementations on Win, Mac, etc.
class TtsPlatformImpl {
 public:
  static TtsPlatformImpl* GetInstance();

  // Returns true if this platform implementation is supported and available.
  virtual bool PlatformImplAvailable() = 0;

  // Some platforms may provide a built-in TTS extension. Returns true
  // if the extension was not previously loaded and is now loading, and
  // false if it's already loaded or if there's no extension to load.
  // Will call TtsController::RetrySpeakingQueuedUtterances when
  // the extension finishes loading.
  virtual bool LoadBuiltInTtsExtension(
      content::BrowserContext* browser_context);

  // Speak the given utterance with the given parameters if possible,
  // and return true on success. Utterance will always be nonempty.
  // If rate, pitch, or volume are -1.0, they will be ignored.
  //
  // The TtsController will only try to speak one utterance at
  // a time. If it wants to interrupt speech, it will always call Stop
  // before speaking again.
  virtual bool Speak(
      int utterance_id,
      const std::string& utterance,
      const std::string& lang,
      const VoiceData& voice,
      const UtteranceContinuousParameters& params) = 0;

  // Stop speaking immediately and return true on success.
  virtual bool StopSpeaking() = 0;

  // Returns whether any speech is on going.
  virtual bool IsSpeaking() = 0;

  // Append information about voices provided by this platform implementation
  // to |out_voices|.
  virtual void GetVoices(std::vector<VoiceData>* out_voices) = 0;

  // Pause the current utterance, if any, until a call to Resume,
  // Speak, or StopSpeaking.
  virtual void Pause() = 0;

  // Resume speaking the current utterance, if it was paused.
  virtual void Resume() = 0;

  // Allows the platform to monitor speech commands and the voices used
  // for each one.
  virtual void WillSpeakUtteranceWithVoice(const Utterance* utterance,
                                           const VoiceData& voice_data);

  virtual std::string error();
  virtual void clear_error();
  virtual void set_error(const std::string& error);

 protected:
  TtsPlatformImpl() {}

  // On some platforms this may be a leaky singleton - do not rely on the
  // destructor being called!  http://crbug.com/122026
  virtual ~TtsPlatformImpl() {}

  std::string error_;

  DISALLOW_COPY_AND_ASSIGN(TtsPlatformImpl);
};

#endif  // CHROME_BROWSER_SPEECH_TTS_PLATFORM_H_