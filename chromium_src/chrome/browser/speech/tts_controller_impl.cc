// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/speech/tts_controller_impl.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/speech/tts_platform.h"

namespace {
// A value to be used to indicate that there is no char index available.
const int kInvalidCharIndex = -1;

// Given a language/region code of the form 'fr-FR', returns just the basic
// language portion, e.g. 'fr'.
std::string TrimLanguageCode(std::string lang) {
  if (lang.size() >= 5 && lang[2] == '-')
    return lang.substr(0, 2);
  else
    return lang;
}

}  // namespace

bool IsFinalTtsEventType(TtsEventType event_type) {
  return (event_type == TTS_EVENT_END ||
          event_type == TTS_EVENT_INTERRUPTED ||
          event_type == TTS_EVENT_CANCELLED ||
          event_type == TTS_EVENT_ERROR);
}

//
// UtteranceContinuousParameters
//


UtteranceContinuousParameters::UtteranceContinuousParameters()
    : rate(-1),
      pitch(-1),
      volume(-1) {}


//
// VoiceData
//


VoiceData::VoiceData()
    : gender(TTS_GENDER_NONE),
      remote(false),
      native(false) {}

VoiceData::~VoiceData() {}


//
// Utterance
//

// static
int Utterance::next_utterance_id_ = 0;

Utterance::Utterance(content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      id_(next_utterance_id_++),
      src_id_(-1),
      gender_(TTS_GENDER_NONE),
      can_enqueue_(false),
      char_index_(0),
      finished_(false) {
  options_.reset(new base::DictionaryValue());
}

Utterance::~Utterance() {
  DCHECK(finished_);
}

void Utterance::OnTtsEvent(TtsEventType event_type,
                           int char_index,
                           const std::string& error_message) {
  if (char_index >= 0)
    char_index_ = char_index;
  if (IsFinalTtsEventType(event_type))
    finished_ = true;

  if (event_delegate_)
    event_delegate_->OnTtsEvent(this, event_type, char_index, error_message);
  if (finished_)
    event_delegate_.reset();
}

void Utterance::Finish() {
  finished_ = true;
}

void Utterance::set_options(const base::Value* options) {
  options_.reset(options->DeepCopy());
}

TtsController* TtsController::GetInstance() {
  return TtsControllerImpl::GetInstance();
}

//
// TtsControllerImpl
//

// static
TtsControllerImpl* TtsControllerImpl::GetInstance() {
  return base::Singleton<TtsControllerImpl>::get();
}

TtsControllerImpl::TtsControllerImpl()
    : current_utterance_(NULL),
      paused_(false),
      platform_impl_(NULL),
      tts_engine_delegate_(NULL) {
}

TtsControllerImpl::~TtsControllerImpl() {
  if (current_utterance_) {
    current_utterance_->Finish();
    delete current_utterance_;
  }

  // Clear any queued utterances too.
  ClearUtteranceQueue(false);  // Don't sent events.
}

void TtsControllerImpl::SpeakOrEnqueue(Utterance* utterance) {
  // If we're paused and we get an utterance that can't be queued,
  // flush the queue but stay in the paused state.
  if (paused_ && !utterance->can_enqueue()) {
    Stop();
    paused_ = true;
    delete utterance;
    return;
  }

  if (paused_ || (IsSpeaking() && utterance->can_enqueue())) {
    utterance_queue_.push(utterance);
  } else {
    Stop();
    SpeakNow(utterance);
  }
}

void TtsControllerImpl::SpeakNow(Utterance* utterance) {
  // Ensure we have all built-in voices loaded. This is a no-op if already
  // loaded.
  bool loaded_built_in =
      GetPlatformImpl()->LoadBuiltInTtsExtension(utterance->browser_context());

  // Get all available voices and try to find a matching voice.
  std::vector<VoiceData> voices;
  GetVoices(utterance->browser_context(), &voices);
  int index = GetMatchingVoice(utterance, voices);

  VoiceData voice;
  if (index != -1) {
    // Select the matching voice.
    voice = voices[index];
  } else {
    // However, if no match was found on a platform without native tts voices,
    // attempt to get a voice based only on the current locale without respect
    // to any supplied voice names.
    std::vector<VoiceData> native_voices;

    if (GetPlatformImpl()->PlatformImplAvailable())
      GetPlatformImpl()->GetVoices(&native_voices);

    if (native_voices.empty() && !voices.empty()) {
      // TODO(dtseng): Notify extension caller of an error.
      utterance->set_voice_name("");
      // TODO(gaochun): Replace the global variable g_browser_process with
      // GetContentClient()->browser() to eliminate the dependency of browser
      // once TTS implementation was moved to content.
      utterance->set_lang(g_browser_process->GetApplicationLocale());
      index = GetMatchingVoice(utterance, voices);

      // If even that fails, just take the first available voice.
      if (index == -1)
        index = 0;
      voice = voices[index];
    } else {
      // Otherwise, simply give native voices a chance to handle this utterance.
      voice.native = true;
    }
  }

  GetPlatformImpl()->WillSpeakUtteranceWithVoice(utterance, voice);

  if (!voice.native) {
#if !defined(OS_ANDROID)
    DCHECK(!voice.extension_id.empty());
    current_utterance_ = utterance;
    utterance->set_extension_id(voice.extension_id);
    if (tts_engine_delegate_)
      tts_engine_delegate_->Speak(utterance, voice);
    bool sends_end_event =
        voice.events.find(TTS_EVENT_END) != voice.events.end();
    if (!sends_end_event) {
      utterance->Finish();
      delete utterance;
      current_utterance_ = NULL;
      SpeakNextUtterance();
    }
#endif
  } else {
    // It's possible for certain platforms to send start events immediately
    // during |speak|.
    current_utterance_ = utterance;
    GetPlatformImpl()->clear_error();
    bool success = GetPlatformImpl()->Speak(
        utterance->id(),
        utterance->text(),
        utterance->lang(),
        voice,
        utterance->continuous_parameters());
    if (!success)
      current_utterance_ = NULL;

    // If the native voice wasn't able to process this speech, see if
    // the browser has built-in TTS that isn't loaded yet.
    if (!success && loaded_built_in) {
      utterance_queue_.push(utterance);
      return;
    }

    if (!success) {
      utterance->OnTtsEvent(TTS_EVENT_ERROR, kInvalidCharIndex,
                            GetPlatformImpl()->error());
      delete utterance;
      return;
    }
  }
}

void TtsControllerImpl::Stop() {
  paused_ = false;
  if (current_utterance_ && !current_utterance_->extension_id().empty()) {
#if !defined(OS_ANDROID)
    if (tts_engine_delegate_)
      tts_engine_delegate_->Stop(current_utterance_);
#endif
  } else {
    GetPlatformImpl()->clear_error();
    GetPlatformImpl()->StopSpeaking();
  }

  if (current_utterance_)
    current_utterance_->OnTtsEvent(TTS_EVENT_INTERRUPTED, kInvalidCharIndex,
                                   std::string());
  FinishCurrentUtterance();
  ClearUtteranceQueue(true);  // Send events.
}

void TtsControllerImpl::Pause() {
  paused_ = true;
  if (current_utterance_ && !current_utterance_->extension_id().empty()) {
#if !defined(OS_ANDROID)
    if (tts_engine_delegate_)
      tts_engine_delegate_->Pause(current_utterance_);
#endif
  } else if (current_utterance_) {
    GetPlatformImpl()->clear_error();
    GetPlatformImpl()->Pause();
  }
}

void TtsControllerImpl::Resume() {
  paused_ = false;
  if (current_utterance_ && !current_utterance_->extension_id().empty()) {
#if !defined(OS_ANDROID)
    if (tts_engine_delegate_)
      tts_engine_delegate_->Resume(current_utterance_);
#endif
  } else if (current_utterance_) {
    GetPlatformImpl()->clear_error();
    GetPlatformImpl()->Resume();
  } else {
    SpeakNextUtterance();
  }
}

void TtsControllerImpl::OnTtsEvent(int utterance_id,
                                        TtsEventType event_type,
                                        int char_index,
                                        const std::string& error_message) {
  // We may sometimes receive completion callbacks "late", after we've
  // already finished the utterance (for example because another utterance
  // interrupted or we got a call to Stop). This is normal and we can
  // safely just ignore these events.
  if (!current_utterance_ || utterance_id != current_utterance_->id()) {
    return;
  }
  current_utterance_->OnTtsEvent(event_type, char_index, error_message);
  if (current_utterance_->finished()) {
    FinishCurrentUtterance();
    SpeakNextUtterance();
  }
}

void TtsControllerImpl::GetVoices(content::BrowserContext* browser_context,
                              std::vector<VoiceData>* out_voices) {
#if !defined(OS_ANDROID)
  if (browser_context && tts_engine_delegate_)
    tts_engine_delegate_->GetVoices(browser_context, out_voices);
#endif

  TtsPlatformImpl* platform_impl = GetPlatformImpl();
  if (platform_impl) {
    // Ensure we have all built-in voices loaded. This is a no-op if already
    // loaded.
    platform_impl->LoadBuiltInTtsExtension(browser_context);
    if (platform_impl->PlatformImplAvailable())
      platform_impl->GetVoices(out_voices);
  }
}

bool TtsControllerImpl::IsSpeaking() {
  return current_utterance_ != NULL || GetPlatformImpl()->IsSpeaking();
}

void TtsControllerImpl::FinishCurrentUtterance() {
  if (current_utterance_) {
    if (!current_utterance_->finished())
      current_utterance_->OnTtsEvent(TTS_EVENT_INTERRUPTED, kInvalidCharIndex,
                                     std::string());
    delete current_utterance_;
    current_utterance_ = NULL;
  }
}

void TtsControllerImpl::SpeakNextUtterance() {
  if (paused_)
    return;

  // Start speaking the next utterance in the queue.  Keep trying in case
  // one fails but there are still more in the queue to try.
  while (!utterance_queue_.empty() && !current_utterance_) {
    Utterance* utterance = utterance_queue_.front();
    utterance_queue_.pop();
    SpeakNow(utterance);
  }
}

void TtsControllerImpl::ClearUtteranceQueue(bool send_events) {
  while (!utterance_queue_.empty()) {
    Utterance* utterance = utterance_queue_.front();
    utterance_queue_.pop();
    if (send_events)
      utterance->OnTtsEvent(TTS_EVENT_CANCELLED, kInvalidCharIndex,
                            std::string());
    else
      utterance->Finish();
    delete utterance;
  }
}

void TtsControllerImpl::SetPlatformImpl(
    TtsPlatformImpl* platform_impl) {
  platform_impl_ = platform_impl;
}

int TtsControllerImpl::QueueSize() {
  return static_cast<int>(utterance_queue_.size());
}

TtsPlatformImpl* TtsControllerImpl::GetPlatformImpl() {
  if (!platform_impl_)
    platform_impl_ = TtsPlatformImpl::GetInstance();
  return platform_impl_;
}

int TtsControllerImpl::GetMatchingVoice(
    const Utterance* utterance, std::vector<VoiceData>& voices) {
  // Make two passes: the first time, do strict language matching
  // ('fr-FR' does not match 'fr-CA'). The second time, do prefix
  // language matching ('fr-FR' matches 'fr' and 'fr-CA')
  for (int pass = 0; pass < 2; ++pass) {
    for (size_t i = 0; i < voices.size(); ++i) {
      const VoiceData& voice = voices[i];

      if (!utterance->extension_id().empty() &&
          utterance->extension_id() != voice.extension_id) {
        continue;
      }

      if (!voice.name.empty() &&
          !utterance->voice_name().empty() &&
          voice.name != utterance->voice_name()) {
        continue;
      }
      if (!voice.lang.empty() && !utterance->lang().empty()) {
        std::string voice_lang = voice.lang;
        std::string utterance_lang = utterance->lang();
        if (pass == 1) {
          voice_lang = TrimLanguageCode(voice_lang);
          utterance_lang = TrimLanguageCode(utterance_lang);
        }
        if (voice_lang != utterance_lang) {
          continue;
        }
      }
      if (voice.gender != TTS_GENDER_NONE &&
          utterance->gender() != TTS_GENDER_NONE &&
          voice.gender != utterance->gender()) {
        continue;
      }

      if (utterance->required_event_types().size() > 0) {
        bool has_all_required_event_types = true;
        for (std::set<TtsEventType>::const_iterator iter =
                 utterance->required_event_types().begin();
             iter != utterance->required_event_types().end();
             ++iter) {
          if (voice.events.find(*iter) == voice.events.end()) {
            has_all_required_event_types = false;
            break;
          }
        }
        if (!has_all_required_event_types)
          continue;
      }

      return static_cast<int>(i);
    }
  }

  return -1;
}

void TtsControllerImpl::VoicesChanged() {
  for (std::set<VoicesChangedDelegate*>::iterator iter =
           voices_changed_delegates_.begin();
       iter != voices_changed_delegates_.end(); ++iter) {
    (*iter)->OnVoicesChanged();
  }
}

void TtsControllerImpl::AddVoicesChangedDelegate(
    VoicesChangedDelegate* delegate) {
  voices_changed_delegates_.insert(delegate);
}

void TtsControllerImpl::RemoveVoicesChangedDelegate(
    VoicesChangedDelegate* delegate) {
  voices_changed_delegates_.erase(delegate);
}

void TtsControllerImpl::SetTtsEngineDelegate(
    TtsEngineDelegate* delegate) {
  tts_engine_delegate_ = delegate;
}

TtsEngineDelegate* TtsControllerImpl::GetTtsEngineDelegate() {
  return tts_engine_delegate_;
}
