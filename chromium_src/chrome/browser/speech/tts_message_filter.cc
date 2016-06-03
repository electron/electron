// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/speech/tts_message_filter.h"

#include "base/bind.h"
#include "base/logging.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"

using content::BrowserThread;

TtsMessageFilter::TtsMessageFilter(int render_process_id,
                                   content::BrowserContext* browser_context)
    : BrowserMessageFilter(TtsMsgStart),
      render_process_id_(render_process_id),
      browser_context_(browser_context),
      weak_ptr_factory_(this) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TtsController::GetInstance()->AddVoicesChangedDelegate(this);

  // Balanced in OnChannelClosingInUIThread() to keep the ref-count be non-zero
  // until all WeakPtr's are invalidated.
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
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&TtsMessageFilter::OnChannelClosingInUIThread, this));
}

void TtsMessageFilter::OnDestruct() const {
  BrowserThread::DeleteOnUIThread::Destruct(this);
}

void TtsMessageFilter::OnInitializeVoiceList() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
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

  std::unique_ptr<Utterance> utterance(new Utterance(browser_context_));
  utterance->set_src_id(request.id);
  utterance->set_text(request.text);
  utterance->set_lang(request.lang);
  utterance->set_voice_name(request.voice);
  utterance->set_can_enqueue(true);

  UtteranceContinuousParameters params;
  params.rate = request.rate;
  params.pitch = request.pitch;
  params.volume = request.volume;
  utterance->set_continuous_parameters(params);

  utterance->set_event_delegate(weak_ptr_factory_.GetWeakPtr());

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
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  OnInitializeVoiceList();
}

void TtsMessageFilter::OnChannelClosingInUIThread() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  TtsController::GetInstance()->RemoveVoicesChangedDelegate(this);

  weak_ptr_factory_.InvalidateWeakPtrs();
  Release();  // Balanced in TtsMessageFilter().
}

TtsMessageFilter::~TtsMessageFilter() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!weak_ptr_factory_.HasWeakPtrs());
  TtsController::GetInstance()->RemoveVoicesChangedDelegate(this);
}