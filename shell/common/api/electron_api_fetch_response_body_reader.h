// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_API_ELECTRON_API_FETCH_RESPONSE_BODY_READER_H_
#define ELECTRON_SHELL_COMMON_API_ELECTRON_API_FETCH_RESPONSE_BODY_READER_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/v8-array-buffer.h"
#include "v8/include/v8-traced-handle.h"

namespace electron {
class TransferableURLLoader;
}

namespace electron::api {

class SimpleURLLoaderWrapper;

class FetchResponseBodyReader final
    : public gin::Wrappable<FetchResponseBodyReader> {
 public:
  static FetchResponseBodyReader* Create(
      v8::Isolate* isolate,
      scoped_refptr<TransferableURLLoader> loader,
      SimpleURLLoaderWrapper* owner);

  FetchResponseBodyReader(v8::Isolate* isolate,
                          scoped_refptr<TransferableURLLoader> loader,
                          SimpleURLLoaderWrapper* owner);
  ~FetchResponseBodyReader() override;

  static const gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "FetchResponseBodyReader"; }
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  void Trace(cppgc::Visitor* visitor) const override;

 private:
  v8::Local<v8::Promise> Read(v8::Local<v8::ArrayBufferView> buffer);
  void OnReadCompleted(int result);

  raw_ptr<v8::Isolate> isolate_;
  scoped_refptr<TransferableURLLoader> loader_;
  cppgc::Member<SimpleURLLoaderWrapper> owner_;
  std::optional<gin_helper::Promise<int>> pending_read_;
  std::shared_ptr<v8::BackingStore> backing_store_;
  v8::TracedReference<v8::ArrayBufferView> read_buffer_;
  gin_helper::SelfKeepAlive<FetchResponseBodyReader> keep_alive_{nullptr};
  gin::WeakCellFactory<FetchResponseBodyReader> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_COMMON_API_ELECTRON_API_FETCH_RESPONSE_BODY_READER_H_
