// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_fetch_response_body_reader.h"

#include <utility>

#include "base/functional/bind.h"
#include "gin/object_template_builder.h"
#include "gin/persistent.h"
#include "net/base/net_errors.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/net/transferable_url_loader.h"
#include "shell/common/v8_util.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

const gin::WrapperInfo FetchResponseBodyReader::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronFetchResponseBodyReader);

// static
FetchResponseBodyReader* FetchResponseBodyReader::Create(
    v8::Isolate* isolate,
    scoped_refptr<TransferableURLLoader> loader) {
  return cppgc::MakeGarbageCollected<FetchResponseBodyReader>(
      isolate->GetCppHeap()->GetAllocationHandle(), isolate, std::move(loader));
}

FetchResponseBodyReader::FetchResponseBodyReader(
    v8::Isolate* isolate,
    scoped_refptr<TransferableURLLoader> loader)
    : isolate_(isolate), loader_(std::move(loader)) {}

FetchResponseBodyReader::~FetchResponseBodyReader() = default;

const gin::WrapperInfo* FetchResponseBodyReader::wrapper_info() const {
  return &kWrapperInfo;
}

const char* FetchResponseBodyReader::GetHumanReadableName() const {
  return "Electron / FetchResponseBodyReader";
}

gin::ObjectTemplateBuilder FetchResponseBodyReader::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<FetchResponseBodyReader>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("read", &FetchResponseBodyReader::Read);
}

void FetchResponseBodyReader::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<FetchResponseBodyReader>::Trace(visitor);
  visitor->Trace(read_buffer_);
  visitor->Trace(weak_factory_);
}

v8::Local<v8::Promise> FetchResponseBodyReader::Read(
    v8::Local<v8::ArrayBufferView> buffer) {
  gin_helper::Promise<int> promise(isolate_);
  auto handle = promise.GetHandle();
  if (pending_read_) {
    std::move(promise).RejectWithErrorMessage(
        "A response body read is already pending");
    return handle;
  }

  pending_read_.emplace(std::move(promise));
  backing_store_ = buffer->Buffer()->GetBackingStore();
  read_buffer_.Reset(isolate_, buffer);
  keep_alive_ = this;
  auto weak_cell = gin::WrapPersistent(
      weak_factory_.GetWeakCell(isolate_->GetCppHeap()->GetAllocationHandle()));
  loader_->Read(electron::util::as_byte_span(buffer),
                base::BindOnce(&FetchResponseBodyReader::OnReadCompleted,
                               std::move(weak_cell)));
  return handle;
}

void FetchResponseBodyReader::OnReadCompleted(int result) {
  auto promise = std::move(*pending_read_);
  pending_read_.reset();
  backing_store_.reset();
  read_buffer_.Reset();
  if (result < 0)
    std::move(promise).RejectWithErrorMessage(net::ErrorToString(result));
  else
    std::move(promise).Resolve(result);
  keep_alive_.Clear();
}

}  // namespace electron::api
