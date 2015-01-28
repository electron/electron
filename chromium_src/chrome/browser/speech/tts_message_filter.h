// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_TTS_MESSAGE_FILTER_H_
#define CHROME_BROWSER_SPEECH_TTS_MESSAGE_FILTER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/speech/tts_controller.h"
#include "chrome/common/tts_messages.h"
#include "content/public/browser/browser_message_filter.h"

namespace content {
class BrowserContext;
}

class TtsMessageFilter
    : public content::BrowserMessageFilter,
      public UtteranceEventDelegate,
      public VoicesChangedDelegate {
 public:
  explicit TtsMessageFilter(int render_process_id,
      content::BrowserContext* browser_context);

  // content::BrowserMessageFilter implementation.
  void OverrideThreadForMessage(
      const IPC::Message& message,
      content::BrowserThread::ID* thread) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelClosing() override;
  void OnDestruct() const override;

  // UtteranceEventDelegate implementation.
  void OnTtsEvent(Utterance* utterance,
                  TtsEventType event_type,
                  int char_index,
                  const std::string& error_message) override;

  // VoicesChangedDelegate implementation.
  void OnVoicesChanged() override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<TtsMessageFilter>;

  virtual ~TtsMessageFilter();

  void OnInitializeVoiceList();
  void OnSpeak(const TtsUtteranceRequest& utterance);
  void OnPause();
  void OnResume();
  void OnCancel();

  void OnChannelClosingInUIThread();

  int render_process_id_;
  content::BrowserContext* browser_context_;

  base::WeakPtrFactory<TtsMessageFilter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TtsMessageFilter);
};

#endif  // CHROME_BROWSER_SPEECH_TTS_MESSAGE_FILTER_H_
