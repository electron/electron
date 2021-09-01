// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_ASAR_ASAR_FILE_VALIDATOR_H
#define SHELL_BROWSER_NET_ASAR_ASAR_FILE_VALIDATOR_H

#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "mojo/public/cpp/system/file_data_source.h"
#include "mojo/public/cpp/system/filtered_data_source.h"
#include "shell/common/asar/archive.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace asar {

class AsarFileValidator : public mojo::FilteredDataSource::Filter {
 public:
  AsarFileValidator(absl::optional<IntegrityPayload> integrity);

  void OnRead(base::span<char> buffer,
              mojo::FileDataSource::ReadResult* result);

  void OnDone();

 protected:
  bool FinishBlock();

 private:
  bool should_validate_ = false;
  absl::optional<IntegrityPayload> integrity_;

  bool done_reading_ = false;
  int current_block_;
  int min_block_;
  int max_block_;
  int current_hash_byte_count_ = 0;
  std::unique_ptr<crypto::SecureHash> current_hash_;

  DISALLOW_COPY_AND_ASSIGN(AsarFileValidator);
};

}  // namespace asar

#endif  // SHELL_BROWSER_NET_ASAR_ASAR_FILE_VALIDATOR_H
