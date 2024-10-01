// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/asar/asar_file_validator.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/logging.h"
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

void AsarFileValidator::EnsureBlockHashExists() {
  if (current_hash_)
    return;

  current_hash_byte_count_ = 0U;
  switch (integrity_.algorithm) {
    case HashAlgorithm::kSHA256:
      current_hash_ = crypto::SecureHash::Create(crypto::SecureHash::SHA256);
      break;
    case HashAlgorithm::kNone:
      CHECK(false);
      break;
  }
}

void AsarFileValidator::OnRead(base::span<char> buffer,
                               mojo::FileDataSource::ReadResult* result) {
  DCHECK(!done_reading_);

  const uint32_t block_size = integrity_.block_size;

  // |buffer| contains the read buffer. |result->bytes_read| is the actual
  // bytes number that |source| read that should be less than buffer.size().
  auto hashme = base::as_bytes(buffer.subspan(0U, result->bytes_read));

  while (!std::empty(hashme)) {
    if (current_block_ > max_block_)
      LOG(FATAL) << "Unexpected block count while validating ASAR file stream";

    EnsureBlockHashExists();

    // hash as many bytes as will fit in the current block.
    const auto n_left_in_block = block_size - current_hash_byte_count_;
    const auto n_now = std::min(n_left_in_block, uint64_t{std::size(hashme)});
    DCHECK_GT(n_now, 0U);
    const auto [hashme_now, hashme_next] = hashme.split_at(n_now);

    current_hash_->Update(hashme_now);
    current_hash_byte_count_ += n_now;
    total_hash_byte_count_ += n_now;

    if (current_hash_byte_count_ == block_size && !FinishBlock())
      LOG(FATAL) << "Streaming ASAR file block hash failed: " << current_block_;

    hashme = hashme_next;
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
    }
    current_hash_->Update(abandoned_buffer);
  }

  auto actual = std::array<uint8_t, crypto::kSHA256Length>{};
  current_hash_->Finish(actual);
  current_hash_.reset();
  current_hash_byte_count_ = 0;

  const auto& expected_hash = integrity_.blocks[current_block_];
  const auto actual_hex_hash = base::ToLowerASCII(base::HexEncode(actual));
  if (expected_hash != actual_hex_hash)
    return false;

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
