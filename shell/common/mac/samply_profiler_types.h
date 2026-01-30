// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_MAC_SAMPLY_PROFILER_TYPES_H_
#define ELECTRON_SHELL_COMMON_MAC_SAMPLY_PROFILER_TYPES_H_

#include <mach/mach.h>
#include <stdint.h>

namespace electron {

// Key for the profiler port in MachPortRendezvous.
inline constexpr uint32_t kMachPortKeyProfiler = 0x70726F66;

// Magic number for samply protocol messages.
inline constexpr uint32_t kSamplyMagic = 0x534D504C;

// Expected minimum size for SamplyReply from Rust.
inline constexpr size_t kSamplyReplyMinSize = 36;

// Expected minimum size for CheckIn message from Rust.
inline constexpr size_t kCheckInMessageMinSize = 48;

// Byte offsets for reading fields from raw message buffers.
inline constexpr size_t kSamplyReplyMagicOffset =
    sizeof(mach_msg_header_t) + sizeof(mach_msg_body_t);
inline constexpr size_t kSamplyReplyStatusOffset =
    kSamplyReplyMagicOffset + sizeof(uint32_t);
inline constexpr size_t kCheckInTokenOffset =
    sizeof(mach_msg_header_t) + sizeof(mach_msg_body_t) +
    sizeof(mach_msg_port_descriptor_t);

// Message structure for connecting a process to samply.
// Matches SimpleSamplyMessage in samply's simple_server.rs.
struct SamplyMessage {
  mach_msg_header_t header;
  mach_msg_body_t body;
  mach_msg_port_descriptor_t reply_port;
  mach_msg_port_descriptor_t task_port;
  uint32_t magic;
  uint32_t pid;
};

// Reply message structure from samply.
// Uses a union to provide both structured access and raw byte access.
union SamplyReplyBuffer {
  mach_msg_header_t header;
  struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    uint32_t magic;
    uint32_t status;
  } structured;
  uint8_t bytes[128];
};

// Buffer for receiving check-in message from samply during handshake.
// Uses a union to provide both structured access and raw byte access.
union CheckInMessageBuffer {
  mach_msg_header_t header;
  struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    mach_msg_port_descriptor_t port;
    uint64_t token;
  } structured;
  uint8_t bytes[256];
};

// Reply message sent to samply containing the receive right.
// Matches ChildPortCheckInReply in samply's simple_server.rs.
struct CheckInReply {
  mach_msg_header_t header;
  mach_msg_body_t body;
  mach_msg_port_descriptor_t port;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_MAC_SAMPLY_PROFILER_TYPES_H_
