// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/v8_value_serializer.h"

#include <utility>
#include <vector>

#include "gin/converter.h"
#include "third_party/blink/public/common/messaging/cloneable_message.h"
#include "v8/include/v8.h"

namespace electron {

namespace {
const uint8_t kVersionTag = 0xFF;
}  // namespace

class V8Serializer : public v8::ValueSerializer::Delegate {
 public:
  explicit V8Serializer(v8::Isolate* isolate)
      : isolate_(isolate), serializer_(isolate, this) {}
  ~V8Serializer() override = default;

  bool Serialize(v8::Local<v8::Value> value, blink::CloneableMessage* out) {
    WriteBlinkEnvelope(19);

    serializer_.WriteHeader();
    bool wrote_value;
    if (!serializer_.WriteValue(isolate_->GetCurrentContext(), value)
             .To(&wrote_value)) {
      isolate_->ThrowException(v8::Exception::Error(
          gin::StringToV8(isolate_, "An object could not be cloned.")));
      return false;
    }
    DCHECK(wrote_value);

    std::pair<uint8_t*, size_t> buffer = serializer_.Release();
    DCHECK_EQ(buffer.first, data_.data());
    out->encoded_message = base::make_span(buffer.first, buffer.second);
    out->owned_encoded_message = std::move(data_);

    return true;
  }

  // v8::ValueSerializer::Delegate
  void* ReallocateBufferMemory(void* old_buffer,
                               size_t size,
                               size_t* actual_size) override {
    DCHECK_EQ(old_buffer, data_.data());
    data_.resize(size);
    *actual_size = data_.capacity();
    return data_.data();
  }

  void FreeBufferMemory(void* buffer) override {
    DCHECK_EQ(buffer, data_.data());
    data_ = {};
  }

  void ThrowDataCloneError(v8::Local<v8::String> message) override {
    isolate_->ThrowException(v8::Exception::Error(message));
  }

 private:
  void WriteTag(uint8_t tag) { serializer_.WriteRawBytes(&tag, 1); }

  void WriteBlinkEnvelope(uint32_t blink_version) {
    // Write a dummy blink version envelope for compatibility with
    // blink::V8ScriptValueSerializer
    WriteTag(kVersionTag);
    serializer_.WriteUint32(blink_version);
  }

  v8::Isolate* isolate_;
  std::vector<uint8_t> data_;
  v8::ValueSerializer serializer_;
};

class V8Deserializer : public v8::ValueDeserializer::Delegate {
 public:
  V8Deserializer(v8::Isolate* isolate, base::span<const uint8_t> data)
      : isolate_(isolate),
        deserializer_(isolate, data.data(), data.size(), this) {}
  V8Deserializer(v8::Isolate* isolate, const blink::CloneableMessage& message)
      : V8Deserializer(isolate, message.encoded_message) {}

  v8::Local<v8::Value> Deserialize() {
    v8::EscapableHandleScope scope(isolate_);
    auto context = isolate_->GetCurrentContext();

    uint32_t blink_version;
    if (!ReadBlinkEnvelope(&blink_version))
      return v8::Null(isolate_);

    bool read_header;
    if (!deserializer_.ReadHeader(context).To(&read_header))
      return v8::Null(isolate_);
    DCHECK(read_header);
    v8::Local<v8::Value> value;
    if (!deserializer_.ReadValue(context).ToLocal(&value))
      return v8::Null(isolate_);
    return scope.Escape(value);
  }

 private:
  bool ReadTag(uint8_t* tag) {
    const void* tag_bytes = nullptr;
    if (!deserializer_.ReadRawBytes(1, &tag_bytes))
      return false;
    *tag = *reinterpret_cast<const uint8_t*>(tag_bytes);
    return true;
  }

  bool ReadBlinkEnvelope(uint32_t* blink_version) {
    // Read a dummy blink version envelope for compatibility with
    // blink::V8ScriptValueDeserializer
    uint8_t tag = 0;
    if (!ReadTag(&tag) || tag != kVersionTag)
      return false;
    if (!deserializer_.ReadUint32(blink_version))
      return false;
    return true;
  }

  v8::Isolate* isolate_;
  v8::ValueDeserializer deserializer_;
};

bool SerializeV8Value(v8::Isolate* isolate,
                      v8::Local<v8::Value> value,
                      blink::CloneableMessage* out) {
  return V8Serializer(isolate).Serialize(value, out);
}

v8::Local<v8::Value> DeserializeV8Value(v8::Isolate* isolate,
                                        const blink::CloneableMessage& in) {
  return V8Deserializer(isolate, in).Deserialize();
}

v8::Local<v8::Value> DeserializeV8Value(v8::Isolate* isolate,
                                        base::span<const uint8_t> data) {
  return V8Deserializer(isolate, data).Deserialize();
}

}  // namespace electron
