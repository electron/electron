// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/speech/tts_message_filter.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/tts_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"

using content::BrowserThread;

TtsMessageFilter::TtsMessageFilter(content::BrowserContext* browser_context)
    : BrowserMessageFilter(TtsMsgStart),
      browser_context_(browser_context),
      valid_(true) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TtsController::GetInstance()->AddVoicesChangedDelegate(this);

  // TODO(dmazzoni): make it so that we can listen for a BrowserContext
  // being destroyed rather than a Profile.  http://crbug.com/444668
  Profile* profile = Profile::FromBrowserContext(browser_context);
  notification_registrar_.Add(this,
                              chrome::NOTIFICATION_PROFILE_DESTROYED,
                              content::Source<Profile>(profile));

  // Balanced in OnChannelClosingInUIThread() to keep the ref-count be non-zero
  // until all pointers to this class are invalidated.
  AddRef();
}

void TtsMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
  switch (message.type()) {
  case TtsHostMsg_InitializeVoiceList::ID:
  case TtsHostMsg_Speak::ID:
  case TtsHostMsg_Pause::ID:
  case TtsHostMsg_Resume::ID:
  case TtsHostMsg_Cancel::ID:
    *thread = BrowserThread::UI;
    break;
  }
}

bool TtsMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(TtsMessageFilter, message)
    IPC_MESSAGE_HANDLER(TtsHostMsg_InitializeVoiceList, OnInitializeVoiceList)
    IPC_MESSAGE_HANDLER(TtsHostMsg_Speak, OnSpeak)
    IPC_MESSAGE_HANDLER(TtsHostMsg_Pause, OnPause)
    IPC_MESSAGE_HANDLER(TtsHostMsg_Resume, OnResume)
    IPC_MESSAGE_HANDLER(TtsHostMsg_Cancel, OnCancel)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void TtsMessageFilter::OnChannelClosing() {
  base::AutoLock lock(mutex_);
  valid_ = false;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&TtsMessageFilter::OnChannelClosingInUIThread, this));
}

bool TtsMessageFilter::Valid() {
  base::AutoLock lock(mutex_);
  return valid_;
}

void TtsMessageFilter::OnDestruct() const {
  {
    base::AutoLock lock(mutex_);
    valid_ = false;
  }
  BrowserThread::DeleteOnUIThread::Destruct(this);
}

TtsMessageFilter::~TtsMessageFilter() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Cleanup();
}

void TtsMessageFilter::OnInitializeVoiceList() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!browser_context_)
    return;

  TtsController* tts_controller = TtsController::GetInstance();
  std::vector<VoiceData> voices;
  tts_controller->GetVoices(browser_context_, &voices);

  std::vector<TtsVoice> out_voices;
  out_voices.resize(voices.size());
  for (size_t i = 0; i < voices.size(); ++i) {
    TtsVoice& out_voice = out_voices[i];
    out_voice.voice_uri = voices[i].name;
    out_voice.name = voices[i].name;
    out_voice.lang = voices[i].lang;
    out_voice.local_service = !voices[i].remote;
    out_voice.is_default = (i == 0);
  }
  Send(new TtsMsg_SetVoiceList(out_voices));
}

void TtsMessageFilter::OnSpeak(const TtsUtteranceRequest& request) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!browser_context_)
    return;

  scoped_ptr<Utterance> utterance(new Utterance(browser_context_));
  utterance->set_src_id(request.id);
  utterance->set_text(request.text);
  utterance->set_lang(request.lang);
  utterance->set_voice_name(request.voice);
  utterance->set_can_enqueue(true);
  utterance->set_continuous_parameters(request.rate,
                                       request.pitch,
                                       request.volume);

  utterance->set_event_delegate(this);

  TtsController::GetInstance()->SpeakOrEnqueue(utterance.release());
}

void TtsMessageFilter::OnPause() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TtsController::GetInstance()->Pause();
}

void TtsMessageFilter::OnResume() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TtsController::GetInstance()->Resume();
}

void TtsMessageFilter::OnCancel() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TtsController::GetInstance()->Stop();
}

void TtsMessageFilter::OnTtsEvent(Utterance* utterance,
                                  TtsEventType event_type,
                                  int char_index,
                                  const std::string& error_message) {
  if (!Valid())
    return;

  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  switch (event_type) {
    case TTS_EVENT_START:
      Send(new TtsMsg_DidStartSpeaking(utterance->src_id()));
      break;
    case TTS_EVENT_END:
      Send(new TtsMsg_DidFinishSpeaking(utterance->src_id()));
      break;
    case TTS_EVENT_WORD:
      Send(new TtsMsg_WordBoundary(utterance->src_id(), char_index));
      break;
    case TTS_EVENT_SENTENCE:
      Send(new TtsMsg_SentenceBoundary(utterance->src_id(), char_index));
      break;
    case TTS_EVENT_MARKER:
      Send(new TtsMsg_MarkerEvent(utterance->src_id(), char_index));
      break;
    case TTS_EVENT_INTERRUPTED:
      Send(new TtsMsg_WasInterrupted(utterance->src_id()));
      break;
    case TTS_EVENT_CANCELLED:
      Send(new TtsMsg_WasCancelled(utterance->src_id()));
      break;
    case TTS_EVENT_ERROR:
      Send(new TtsMsg_SpeakingErrorOccurred(
          utterance->src_id(), error_message));
      break;
    case TTS_EVENT_PAUSE:
      Send(new TtsMsg_DidPauseSpeaking(utterance->src_id()));
      break;
    case TTS_EVENT_RESUME:
      Send(new TtsMsg_DidResumeSpeaking(utterance->src_id()));
      break;
  }
}

void TtsMessageFilter::OnVoicesChanged() {
  if (!Valid())
    return;

  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  OnInitializeVoiceList();
}

void TtsMessageFilter::OnChannelClosingInUIThread() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Cleanup();
  Release();  // Balanced in TtsMessageFilter().
}

void TtsMessageFilter::Cleanup() {
  TtsController::GetInstance()->RemoveVoicesChangedDelegate(this);
  TtsController::GetInstance()->RemoveUtteranceEventDelegate(this);
}

void TtsMessageFilter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  browser_context_ = nullptr;
  notification_registrar_.RemoveAll();
}
