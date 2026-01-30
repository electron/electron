// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/mac/samply_profiler_client.h"

#include <unistd.h>

#include "base/apple/mach_logging.h"
#include "base/apple/mach_port_rendezvous.h"
#include "base/apple/scoped_mach_port.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "shell/common/mac/samply_profiler_types.h"

namespace electron {

bool ConnectToSamplyProfiler(mach_port_t profiler_port) {
  if (profiler_port == MACH_PORT_NULL) {
    return false;
  }

  base::apple::ScopedMachReceiveRight reply_port;
  kern_return_t kr = mach_port_allocate(
      mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
      base::apple::ScopedMachReceiveRight::Receiver(reply_port).get());
  if (kr != KERN_SUCCESS) {
    MACH_LOG(WARNING, kr) << "mach_port_allocate for profiler reply";
    return false;
  }

  SamplyMessage msg = {};
  msg.header.msgh_bits =
      MACH_MSGH_BITS_COMPLEX | MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
  msg.header.msgh_size = sizeof(SamplyMessage);
  msg.header.msgh_remote_port = profiler_port;
  msg.header.msgh_local_port = MACH_PORT_NULL;
  msg.header.msgh_id = 0;

  msg.body.msgh_descriptor_count = 2;

  msg.reply_port.name = reply_port.get();
  msg.reply_port.disposition = MACH_MSG_TYPE_MAKE_SEND;
  msg.reply_port.type = MACH_MSG_PORT_DESCRIPTOR;

  msg.task_port.name = mach_task_self();
  msg.task_port.disposition = MACH_MSG_TYPE_COPY_SEND;
  msg.task_port.type = MACH_MSG_PORT_DESCRIPTOR;

  msg.magic = kSamplyMagic;
  msg.pid = static_cast<uint32_t>(getpid());

  kr = mach_msg(&msg.header, MACH_SEND_MSG, sizeof(SamplyMessage), 0,
                MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(WARNING, kr) << "mach_msg send to samply profiler";
    return false;
  }

  SamplyReplyBuffer reply = {};
  reply.header.msgh_size = sizeof(SamplyReplyBuffer);
  reply.header.msgh_local_port = reply_port.get();

  kr = mach_msg(&reply.header, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0,
                sizeof(SamplyReplyBuffer), reply_port.get(), 5000,
                MACH_PORT_NULL);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(WARNING, kr) << "mach_msg receive from samply profiler";
    return false;
  }

  if (reply.header.msgh_size < kSamplyReplyMinSize) {
    LOG(ERROR) << "Samply profiler: reply too small, size="
               << reply.header.msgh_size
               << " expected >= " << kSamplyReplyMinSize;
    return false;
  }

  // Read magic and status from bytes[] at known offsets.
  uint32_t magic = base::U32FromNativeEndian(
      base::span(reply.bytes)
          .subspan<kSamplyReplyMagicOffset, sizeof(uint32_t)>());
  uint32_t status = base::U32FromNativeEndian(
      base::span(reply.bytes)
          .subspan<kSamplyReplyStatusOffset, sizeof(uint32_t)>());

  if (magic != kSamplyMagic) {
    LOG(ERROR) << "Samply profiler: invalid reply magic";
    return false;
  }

  if (status != 0) {
    LOG(ERROR) << "Samply profiler: error status " << status;
    return false;
  }

  return true;
}

void MaybeConnectToSamplyProfiler() {
  auto* client = base::MachPortRendezvousClient::GetInstance();
  if (!client) {
    // Not available (e.g., no rendezvous server or running standalone)
    return;
  }

  auto profiler_port = client->TakeSendRight(kMachPortKeyProfiler);
  if (!profiler_port.is_valid()) {
    return;
  }

  if (ConnectToSamplyProfiler(profiler_port.get())) {
    VLOG(1) << "Connected to samply profiler (child process)";
  }
}

}  // namespace electron
