// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/utility/atom_content_utility_client.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "chrome/common/chrome_utility_messages.h"
#include "chrome/utility/utility_message_handler.h"
#include "content/public/common/content_switches.h"
#include "content/public/utility/utility_thread.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message_macros.h"


#if defined(OS_WIN)
#include "chrome/utility/printing_handler_win.h"
#endif


namespace {

bool Send(IPC::Message* message) {
  return content::UtilityThread::Get()->Send(message);
}

}  // namespace

namespace atom {

int64_t AtomContentUtilityClient::max_ipc_message_size_ =
    IPC::Channel::kMaximumMessageSize;

AtomContentUtilityClient::AtomContentUtilityClient()
    : filter_messages_(false) {
#if defined(OS_WIN)
  handlers_.push_back(new printing::PrintingHandlerWin());
#endif
}

AtomContentUtilityClient::~AtomContentUtilityClient() {
}

void AtomContentUtilityClient::UtilityThreadStarted() {
}

bool AtomContentUtilityClient::OnMessageReceived(
    const IPC::Message& message) {
  if (filter_messages_ && !ContainsKey(message_id_whitelist_, message.type()))
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AtomContentUtilityClient, message)
    IPC_MESSAGE_HANDLER(ChromeUtilityMsg_StartupPing, OnStartupPing)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  for (auto it = handlers_.begin(); !handled && it != handlers_.end(); ++it) {
    handled = (*it)->OnMessageReceived(message);
  }

  return handled;
}

void AtomContentUtilityClient::OnStartupPing() {
  Send(new ChromeUtilityHostMsg_ProcessStarted);
  // Don't release the process, we assume further messages are on the way.
}

}  // namespace atom
