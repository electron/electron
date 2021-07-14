// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/heap_snapshot.h"

#include "base/files/file.h"
#include "v8/include/v8-profiler.h"
#include "v8/include/v8.h"

namespace {

class HeapSnapshotOutputStream : public v8::OutputStream {
 public:
  explicit HeapSnapshotOutputStream(base::File* file) : file_(file) {
    DCHECK(file_);
  }

  bool IsComplete() const { return is_complete_; }

  // v8::OutputStream
  int GetChunkSize() override { return 65536; }
  void EndOfStream() override { is_complete_ = true; }

  v8::OutputStream::WriteResult WriteAsciiChunk(char* data, int size) override {
    auto bytes_written = file_->WriteAtCurrentPos(data, size);
    return bytes_written == size ? kContinue : kAbort;
  }

 private:
  base::File* file_ = nullptr;
  bool is_complete_ = false;
};

}  // namespace

namespace electron {

bool TakeHeapSnapshot(v8::Isolate* isolate, base::File* file) {
  DCHECK(isolate);
  DCHECK(file);

  if (!file->IsValid())
    return false;

  auto* snapshot = isolate->GetHeapProfiler()->TakeHeapSnapshot();
  if (!snapshot)
    return false;

  HeapSnapshotOutputStream stream(file);
  snapshot->Serialize(&stream, v8::HeapSnapshot::kJSON);

  const_cast<v8::HeapSnapshot*>(snapshot)->Delete();

  return stream.IsComplete();
}

}  // namespace electron
