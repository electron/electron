// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_data_pipe_holder.h"

#include "base/strings/string_number_conversions.h"
#include "shell/common/key_weak_map.h"

namespace electron {

namespace api {

namespace {

// Incremental ID.
int g_next_id = 0;

// Map that manages all the DataPipeHolder objects.
KeyWeakMap<std::string> g_weak_map;

}  // namespace

gin::WrapperInfo DataPipeHolder::kWrapperInfo = {gin::kEmbedderNativeGin};

DataPipeHolder::DataPipeHolder(const network::DataElement& element)
    : id_(base::NumberToString(++g_next_id)),
      data_pipe_(element.CloneDataPipeGetter()) {}

DataPipeHolder::~DataPipeHolder() = default;

network::mojom::DataPipeGetterPtr DataPipeHolder::Release() {
  return std::move(data_pipe_);
}

// static
gin::Handle<DataPipeHolder> DataPipeHolder::Create(
    v8::Isolate* isolate,
    const network::DataElement& element) {
  auto handle = gin::CreateHandle(isolate, new DataPipeHolder(element));
  g_weak_map.Set(isolate, handle->id(),
                 handle->GetWrapper(isolate).ToLocalChecked());
  return handle;
}

// static
gin::Handle<DataPipeHolder> DataPipeHolder::From(v8::Isolate* isolate,
                                                 const std::string& id) {
  v8::MaybeLocal<v8::Object> object = g_weak_map.Get(isolate, id);
  if (!object.IsEmpty()) {
    gin::Handle<DataPipeHolder> handle;
    if (gin::ConvertFromV8(isolate, object.ToLocalChecked(), &handle))
      return handle;
  }
  return gin::Handle<DataPipeHolder>();
}

}  // namespace api

}  // namespace electron
