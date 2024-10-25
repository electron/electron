// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/heap_snapshot.h"

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/memory/raw_ptr.h"
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
    const uint8_t* udata = reinterpret_cast<const uint8_t*>(data);
    const size_t usize = static_cast<size_t>(std::max(0, size));
    // SAFETY: since WriteAsciiChunk() only gives us data + size, our
    // UNSAFE_BUFFERS macro call is unavoidable here. It can be removed
    // if/when v8 changes WriteAsciiChunk() to pass a v8::MemorySpan.
    const auto data_span = UNSAFE_BUFFERS(base::span(udata, usize));
    return file_->WriteAtCurrentPosAndCheck(data_span) ? kContinue : kAbort;
  }

 private:
  raw_ptr<base::File> file_ = nullptr;
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
