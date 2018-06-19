// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/network_converter.h"

#include <string>
#include <utility>

#include "atom/common/native_mate_converters/value_converter.h"
#include "native_mate/dictionary.h"
#include "services/network/public/cpp/resource_request_body.h"

namespace mate {

// static
v8::Local<v8::Value>
Converter<scoped_refptr<network::ResourceRequestBody>>::ToV8(
    v8::Isolate* isolate,
    const scoped_refptr<network::ResourceRequestBody>& val) {
  if (!val)
    return v8::Null(isolate);
  auto list = std::make_unique<base::ListValue>();
  for (const auto& element : *(val->elements())) {
    auto post_data_dict = std::make_unique<base::DictionaryValue>();
    auto type = element.type();
    if (type == network::DataElement::TYPE_BYTES) {
      auto bytes = base::Value::CreateWithCopiedBuffer(
          element.bytes(), static_cast<size_t>(element.length()));
      post_data_dict->SetString("type", "rawData");
      post_data_dict->Set("bytes", std::move(bytes));
    } else if (type == network::DataElement::TYPE_FILE) {
      post_data_dict->SetString("type", "file");
      post_data_dict->SetKey("filePath",
                             base::Value(element.path().AsUTF8Unsafe()));
      post_data_dict->SetInteger("offset", static_cast<int>(element.offset()));
      post_data_dict->SetInteger("length", static_cast<int>(element.length()));
      post_data_dict->SetDouble(
          "modificationTime", element.expected_modification_time().ToDoubleT());
    } else if (type == network::DataElement::TYPE_BLOB) {
      post_data_dict->SetString("type", "blob");
      post_data_dict->SetString("blobUUID", element.blob_uuid());
    }
    list->Append(std::move(post_data_dict));
  }
  return ConvertToV8(isolate, *list);
}

// static
bool Converter<scoped_refptr<network::ResourceRequestBody>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    scoped_refptr<network::ResourceRequestBody>* out) {
  auto list = std::make_unique<base::ListValue>();
  if (!ConvertFromV8(isolate, val, list.get()))
    return false;
  *out = new network::ResourceRequestBody();
  for (size_t i = 0; i < list->GetSize(); ++i) {
    base::DictionaryValue* dict = nullptr;
    std::string type;
    if (!list->GetDictionary(i, &dict))
      return false;
    dict->GetString("type", &type);
    if (type == "rawData") {
      base::Value* bytes = nullptr;
      dict->GetBinary("bytes", &bytes);
      (*out)->AppendBytes(bytes->GetBlob().data(), bytes->GetBlob().size());
    } else if (type == "file") {
      std::string file;
      int offset = 0, length = -1;
      double modification_time = 0.0;
      dict->GetStringWithoutPathExpansion("filePath", &file);
      dict->GetInteger("offset", &offset);
      dict->GetInteger("file", &length);
      dict->GetDouble("modificationTime", &modification_time);
      (*out)->AppendFileRange(base::FilePath::FromUTF8Unsafe(file),
                              static_cast<uint64_t>(offset),
                              static_cast<uint64_t>(length),
                              base::Time::FromDoubleT(modification_time));
    } else if (type == "blob") {
      std::string uuid;
      dict->GetString("blobUUID", &uuid);
      (*out)->AppendBlob(uuid);
    }
  }
  return true;
}

}  // namespace mate
