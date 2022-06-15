// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/v8_value_serializer.h"

#include <utility>
#include <vector>

#include "gin/converter.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "skia/public/mojom/bitmap.mojom.h"
#include "third_party/blink/public/common/messaging/cloneable_message.h"
#include "ui/gfx/image/image_skia.h"
#include "v8/include/v8.h"

namespace electron {

namespace {
enum SerializationTag { kNativeImageTag = 'i', kVersionTag = 0xFF };
}  // namespace

class V8Serializer : public v8::ValueSerializer::Delegate {
 public:
  explicit V8Serializer(v8::Isolate* isolate)
      : isolate_(isolate), serializer_(isolate, this) {}
  ~V8Serializer() override = default;

  bool Serialize(v8::Local<v8::Value> value, blink::CloneableMessage* out) {
    gin_helper::MicrotasksScope microtasks_scope(
        isolate_, v8::MicrotasksScope::kDoNotRunMicrotasks);
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

  v8::Maybe<bool> WriteHostObject(v8::Isolate* isolate,
                                  v8::Local<v8::Object> object) override {
    api::NativeImage* native_image;
    if (gin::ConvertFromV8(isolate, object, &native_image)) {
      // Serialize the NativeImage
      WriteTag(kNativeImageTag);
      gfx::ImageSkia image = native_image->image().AsImageSkia();
      std::vector<gfx::ImageSkiaRep> image_reps = image.image_reps();
      serializer_.WriteUint32(image_reps.size());
      for (const auto& rep : image_reps) {
        serializer_.WriteDouble(rep.scale());
        const SkBitmap& bitmap = rep.GetBitmap();
        std::vector<uint8_t> bytes =
            skia::mojom::InlineBitmap::Serialize(&bitmap);
        serializer_.WriteUint32(bytes.size());
        serializer_.WriteRawBytes(bytes.data(), bytes.size());
      }
      return v8::Just(true);
    } else {
      return v8::ValueSerializer::Delegate::WriteHostObject(isolate, object);
    }
  }

  void ThrowDataCloneError(v8::Local<v8::String> message) override {
    isolate_->ThrowException(v8::Exception::Error(message));
  }

 private:
  void WriteTag(SerializationTag tag) { serializer_.WriteRawBytes(&tag, 1); }

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

  v8::MaybeLocal<v8::Object> ReadHostObject(v8::Isolate* isolate) override {
    uint8_t tag = 0;
    if (!ReadTag(&tag))
      return v8::ValueDeserializer::Delegate::ReadHostObject(isolate);
    switch (tag) {
      case kNativeImageTag:
        if (api::NativeImage* native_image = ReadNativeImage(isolate))
          return native_image->GetWrapper(isolate);
        break;
    }
    // Throws an exception.
    return v8::ValueDeserializer::Delegate::ReadHostObject(isolate);
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

  api::NativeImage* ReadNativeImage(v8::Isolate* isolate) {
    gfx::ImageSkia image_skia;
    uint32_t num_reps = 0;
    if (!deserializer_.ReadUint32(&num_reps))
      return nullptr;
    for (uint32_t i = 0; i < num_reps; i++) {
      double scale = 0.0;
      if (!deserializer_.ReadDouble(&scale))
        return nullptr;
      uint32_t bitmap_size_bytes = 0;
      if (!deserializer_.ReadUint32(&bitmap_size_bytes))
        return nullptr;
      const void* bitmap_data = nullptr;
      if (!deserializer_.ReadRawBytes(bitmap_size_bytes, &bitmap_data))
        return nullptr;
      SkBitmap bitmap;
      if (!skia::mojom::InlineBitmap::Deserialize(bitmap_data,
                                                  bitmap_size_bytes, &bitmap))
        return nullptr;
      image_skia.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale));
    }
    gfx::Image image(image_skia);
    return new api::NativeImage(isolate, image);
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
