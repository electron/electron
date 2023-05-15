// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/asar/asar_file_validator.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/sha2.h"

namespace asar {

AsarFileValidator::AsarFileValidator(IntegrityPayload integrity,
                                     base::File file)
    : file_(std::move(file)), integrity_(std::move(integrity)) {
  current_block_ = 0;
  max_block_ = integrity_.blocks.size() - 1;
}

AsarFileValidator::~AsarFileValidator() = default;

void AsarFileValidator::OnRead(base::span<char> buffer,
                               mojo::FileDataSource::ReadResult* result) {
  DCHECK(!done_reading_);

  uint64_t buffer_size = result->bytes_read;

  // Compute how many bytes we should hash, and add them to the current hash.
  uint32_t block_size = integrity_.block_size;
  uint64_t bytes_added = 0;
  while (bytes_added < buffer_size) {
    if (current_block_ > max_block_) {
      LOG(FATAL)
          << "Unexpected number of blocks while validating ASAR file stream";
      return;
    }

    // Create a hash if we don't have one yet
    if (!current_hash_) {
      current_hash_byte_count_ = 0;
      switch (integrity_.algorithm) {
        case HashAlgorithm::kSHA256:
          current_hash_ =
              crypto::SecureHash::Create(crypto::SecureHash::SHA256);
          break;
        case HashAlgorithm::kNone:
          CHECK(false);
          break;
      }
    }

    // Compute how many bytes we should hash, and add them to the current hash.
    // We need to either add just enough bytes to fill up a block (block_size -
    // current_bytes) or use every remaining byte (buffer_size - bytes_added)
    int bytes_to_hash = std::min(block_size - current_hash_byte_count_,
                                 buffer_size - bytes_added);
    DCHECK_GT(bytes_to_hash, 0);
    current_hash_->Update(buffer.data() + bytes_added, bytes_to_hash);
    bytes_added += bytes_to_hash;
    current_hash_byte_count_ += bytes_to_hash;
    total_hash_byte_count_ += bytes_to_hash;

    if (current_hash_byte_count_ == block_size && !FinishBlock()) {
      LOG(FATAL) << "Failed to validate block while streaming ASAR file: "
                 << current_block_;
      return;
    }
  }
}

bool AsarFileValidator::FinishBlock() {
  if (current_hash_byte_count_ == 0) {
    if (!done_reading_ || current_block_ > max_block_) {
      return true;
    }
  }

  if (!current_hash_) {
    // This happens when we fail to read the resource. Compute empty content's
    // hash in this case.
    current_hash_ = crypto::SecureHash::Create(crypto::SecureHash::SHA256);
  }

  uint8_t actual[crypto::kSHA256Length];

  // If the file reader is done we need to make sure we've either read up to the
  // end of the file (the check below) or up to the end of a block_size byte
  // boundary. If the below check fails we compute the next block boundary, how
  // many bytes are needed to get there and then we manually read those bytes
  // from our own file handle ensuring the data producer is unaware but we can
  // validate the hash still.
  if (done_reading_ &&
      total_hash_byte_count_ - extra_read_ != read_max_ - read_start_) {
    uint64_t bytes_needed = std::min(
        integrity_.block_size - current_hash_byte_count_,
        read_max_ - read_start_ - total_hash_byte_count_ + extra_read_);
    uint64_t offset = read_start_ + total_hash_byte_count_ - extra_read_;
    std::vector<uint8_t> abandoned_buffer(bytes_needed);
    if (!file_.ReadAndCheck(offset, abandoned_buffer)) {
      LOG(FATAL) << "Failed to read required portion of streamed ASAR archive";
      return false;
    }

    current_hash_->Update(&abandoned_buffer.front(), bytes_needed);
  }

  current_hash_->Finish(actual, sizeof(actual));
  current_hash_.reset();
  current_hash_byte_count_ = 0;

  const std::string expected_hash = integrity_.blocks[current_block_];
  const std::string actual_hex_hash =
      base::ToLowerASCII(base::HexEncode(actual, sizeof(actual)));

  if (expected_hash != actual_hex_hash) {
    return false;
  }

  current_block_++;

  return true;
}

void AsarFileValidator::OnDone() {
  DCHECK(!done_reading_);
  done_reading_ = true;
  if (!FinishBlock()) {
    LOG(FATAL) << "Failed to validate block while ending ASAR file stream: "
               << current_block_;
  }
}

void AsarFileValidator::SetRange(uint64_t read_start,
                                 uint64_t extra_read,
                                 uint64_t read_max) {
  read_start_ = read_start;
  extra_read_ = extra_read;
  read_max_ = read_max;
}

void AsarFileValidator::SetCurrentBlock(int current_block) {
  current_block_ = current_block;
}

}  // namespace asar
