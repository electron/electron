// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include <map>

#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "chrome/browser/speech/tts_platform.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"

#include "library_loaders/libspeechd.h"

using content::BrowserThread;

namespace {

const char kNotSupportedError[] =
    "Native speech synthesis not supported on this platform.";

struct SPDChromeVoice {
  std::string name;
  std::string module;
};

}  // namespace

class TtsPlatformImplLinux : public TtsPlatformImpl {
 public:
  bool PlatformImplAvailable() override;
  bool Speak(int utterance_id,
             const std::string& utterance,
             const std::string& lang,
             const VoiceData& voice,
             const UtteranceContinuousParameters& params) override;
  bool StopSpeaking() override;
  void Pause() override;
  void Resume() override;
  bool IsSpeaking() override;
  void GetVoices(std::vector<VoiceData>* out_voices) override;

  void OnSpeechEvent(SPDNotificationType type);

  // Get the single instance of this class.
  static TtsPlatformImplLinux* GetInstance();

 private:
  TtsPlatformImplLinux();
  ~TtsPlatformImplLinux() override;

  // Initiate the connection with the speech dispatcher.
  void Initialize();

  // Resets the connection with speech dispatcher.
  void Reset();

  static void NotificationCallback(size_t msg_id,
                                   size_t client_id,
                                   SPDNotificationType type);

  static void IndexMarkCallback(size_t msg_id,
                                size_t client_id,
                                SPDNotificationType state,
                                char* index_mark);

  static SPDNotificationType current_notification_;

  base::Lock initialization_lock_;
  LibSpeechdLoader libspeechd_loader_;
  SPDConnection* conn_;

  // These apply to the current utterance only.
  std::string utterance_;
  int utterance_id_;

  // Map a string composed of a voicename and module to the voicename. Used to
  // uniquely identify a voice across all available modules.
  std::unique_ptr<std::map<std::string, SPDChromeVoice> > all_native_voices_;

  friend struct base::DefaultSingletonTraits<TtsPlatformImplLinux>;

  DISALLOW_COPY_AND_ASSIGN(TtsPlatformImplLinux);
};

// static
SPDNotificationType TtsPlatformImplLinux::current_notification_ =
    SPD_EVENT_END;

TtsPlatformImplLinux::TtsPlatformImplLinux()
    : utterance_id_(0) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kEnableSpeechDispatcher))
    return;

  BrowserThread::PostTask(BrowserThread::FILE,
                          FROM_HERE,
                          base::Bind(&TtsPlatformImplLinux::Initialize,
                                     base::Unretained(this)));
}

void TtsPlatformImplLinux::Initialize() {
  base::AutoLock lock(initialization_lock_);

  if (!libspeechd_loader_.Load("libspeechd.so.2"))
    return;

  {
    // spd_open has memory leaks which are hard to suppress.
    // http://crbug.com/317360
    ANNOTATE_SCOPED_MEMORY_LEAK;
    conn_ = libspeechd_loader_.spd_open(
        "chrome", "extension_api", NULL, SPD_MODE_THREADED);
  }
  if (!conn_)
    return;

  // Register callbacks for all events.
  conn_->callback_begin =
    conn_->callback_end =
    conn_->callback_cancel =
    conn_->callback_pause =
    conn_->callback_resume =
    &NotificationCallback;

  conn_->callback_im = &IndexMarkCallback;

  libspeechd_loader_.spd_set_notification_on(conn_, SPD_BEGIN);
  libspeechd_loader_.spd_set_notification_on(conn_, SPD_END);
  libspeechd_loader_.spd_set_notification_on(conn_, SPD_CANCEL);
  libspeechd_loader_.spd_set_notification_on(conn_, SPD_PAUSE);
  libspeechd_loader_.spd_set_notification_on(conn_, SPD_RESUME);
}

TtsPlatformImplLinux::~TtsPlatformImplLinux() {
  base::AutoLock lock(initialization_lock_);
  if (conn_) {
    libspeechd_loader_.spd_close(conn_);
    conn_ = NULL;
  }
}

void TtsPlatformImplLinux::Reset() {
  base::AutoLock lock(initialization_lock_);
  if (conn_)
    libspeechd_loader_.spd_close(conn_);
  conn_ = libspeechd_loader_.spd_open(
      "chrome", "extension_api", NULL, SPD_MODE_THREADED);
}

bool TtsPlatformImplLinux::PlatformImplAvailable() {
  if (!initialization_lock_.Try())
    return false;
  bool result = libspeechd_loader_.loaded() && (conn_ != NULL);
  initialization_lock_.Release();
  return result;
}

bool TtsPlatformImplLinux::Speak(
    int utterance_id,
    const std::string& utterance,
    const std::string& lang,
    const VoiceData& voice,
    const UtteranceContinuousParameters& params) {
  if (!PlatformImplAvailable()) {
    error_ = kNotSupportedError;
    return false;
  }

  // Speech dispatcher's speech params are around 3x at either limit.
  float rate = params.rate > 3 ? 3 : params.rate;
  rate = params.rate < 0.334 ? 0.334 : rate;
  float pitch = params.pitch > 3 ? 3 : params.pitch;
  pitch = params.pitch < 0.334 ? 0.334 : pitch;

  std::map<std::string, SPDChromeVoice>::iterator it =
      all_native_voices_->find(voice.name);
  if (it != all_native_voices_->end()) {
    libspeechd_loader_.spd_set_output_module(conn_, it->second.module.c_str());
    libspeechd_loader_.spd_set_synthesis_voice(conn_, it->second.name.c_str());
  }

  // Map our multiplicative range to Speech Dispatcher's linear range.
  // .334 = -100.
  // 3 = 100.
  libspeechd_loader_.spd_set_voice_rate(conn_, 100 * log10(rate) / log10(3));
  libspeechd_loader_.spd_set_voice_pitch(conn_, 100 * log10(pitch) / log10(3));

  // Support languages other than the default
  if (!lang.empty())
    libspeechd_loader_.spd_set_language(conn_, lang.c_str());

  utterance_ = utterance;
  utterance_id_ = utterance_id;

  if (libspeechd_loader_.spd_say(conn_, SPD_TEXT, utterance.c_str()) == -1) {
    Reset();
    return false;
  }
  return true;
}

bool TtsPlatformImplLinux::StopSpeaking() {
  if (!PlatformImplAvailable())
    return false;
  if (libspeechd_loader_.spd_stop(conn_) == -1) {
    Reset();
    return false;
  }
  return true;
}

void TtsPlatformImplLinux::Pause() {
  if (!PlatformImplAvailable())
    return;
  libspeechd_loader_.spd_pause(conn_);
}

void TtsPlatformImplLinux::Resume() {
  if (!PlatformImplAvailable())
    return;
  libspeechd_loader_.spd_resume(conn_);
}

bool TtsPlatformImplLinux::IsSpeaking() {
  return current_notification_ == SPD_EVENT_BEGIN;
}

void TtsPlatformImplLinux::GetVoices(
    std::vector<VoiceData>* out_voices) {
  if (!all_native_voices_.get()) {
    all_native_voices_.reset(new std::map<std::string, SPDChromeVoice>());
    char** modules = libspeechd_loader_.spd_list_modules(conn_);
    if (!modules)
      return;
    for (int i = 0; modules[i]; i++) {
      char* module = modules[i];
      libspeechd_loader_.spd_set_output_module(conn_, module);
      SPDVoice** native_voices =
          libspeechd_loader_.spd_list_synthesis_voices(conn_);
      if (!native_voices) {
        free(module);
        continue;
      }
      for (int j = 0; native_voices[j]; j++) {
        SPDVoice* native_voice = native_voices[j];
        SPDChromeVoice native_data;
        native_data.name = native_voice->name;
        native_data.module = module;
        std::string key;
        key.append(native_data.name);
        key.append(" ");
        key.append(native_data.module);
        all_native_voices_->insert(
            std::pair<std::string, SPDChromeVoice>(key, native_data));
        free(native_voices[j]);
      }
      free(modules[i]);
    }
  }

  for (std::map<std::string, SPDChromeVoice>::iterator it =
           all_native_voices_->begin();
       it != all_native_voices_->end();
       it++) {
    out_voices->push_back(VoiceData());
    VoiceData& voice = out_voices->back();
    voice.native = true;
    voice.name = it->first;
    voice.events.insert(TTS_EVENT_START);
    voice.events.insert(TTS_EVENT_END);
    voice.events.insert(TTS_EVENT_CANCELLED);
    voice.events.insert(TTS_EVENT_MARKER);
    voice.events.insert(TTS_EVENT_PAUSE);
    voice.events.insert(TTS_EVENT_RESUME);
  }
}

void TtsPlatformImplLinux::OnSpeechEvent(SPDNotificationType type) {
  TtsController* controller = TtsController::GetInstance();
  switch (type) {
  case SPD_EVENT_BEGIN:
    controller->OnTtsEvent(utterance_id_, TTS_EVENT_START, 0, std::string());
    break;
  case SPD_EVENT_RESUME:
    controller->OnTtsEvent(utterance_id_, TTS_EVENT_RESUME, 0, std::string());
    break;
  case SPD_EVENT_END:
    controller->OnTtsEvent(
        utterance_id_, TTS_EVENT_END, utterance_.size(), std::string());
    break;
  case SPD_EVENT_PAUSE:
    controller->OnTtsEvent(
        utterance_id_, TTS_EVENT_PAUSE, utterance_.size(), std::string());
    break;
  case SPD_EVENT_CANCEL:
    controller->OnTtsEvent(
        utterance_id_, TTS_EVENT_CANCELLED, 0, std::string());
    break;
  case SPD_EVENT_INDEX_MARK:
    controller->OnTtsEvent(utterance_id_, TTS_EVENT_MARKER, 0, std::string());
    break;
  }
}

// static
void TtsPlatformImplLinux::NotificationCallback(
    size_t msg_id, size_t client_id, SPDNotificationType type) {
  // We run Speech Dispatcher in threaded mode, so these callbacks should always
  // be in a separate thread.
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    current_notification_ = type;
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&TtsPlatformImplLinux::OnSpeechEvent,
                   base::Unretained(TtsPlatformImplLinux::GetInstance()),
                   type));
  }
}

// static
void TtsPlatformImplLinux::IndexMarkCallback(size_t msg_id,
                                                      size_t client_id,
                                                      SPDNotificationType state,
                                                      char* index_mark) {
  // TODO(dtseng): index_mark appears to specify an index type supplied by a
  // client. Need to explore how this is used before hooking it up with existing
  // word, sentence events.
  // We run Speech Dispatcher in threaded mode, so these callbacks should always
  // be in a separate thread.
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    current_notification_ = state;
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&TtsPlatformImplLinux::OnSpeechEvent,
        base::Unretained(TtsPlatformImplLinux::GetInstance()),
        state));
  }
}

// static
TtsPlatformImplLinux* TtsPlatformImplLinux::GetInstance() {
  return base::Singleton<
      TtsPlatformImplLinux,
      base::LeakySingletonTraits<TtsPlatformImplLinux>>::get();
}

// static
TtsPlatformImpl* TtsPlatformImpl::GetInstance() {
  return TtsPlatformImplLinux::GetInstance();
}
