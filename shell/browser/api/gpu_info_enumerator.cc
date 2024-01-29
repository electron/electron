// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/gpu_info_enumerator.h"

#include <utility>

namespace electron {

GPUInfoEnumerator::GPUInfoEnumerator() = default;

GPUInfoEnumerator::~GPUInfoEnumerator() = default;

void GPUInfoEnumerator::AddInt64(const char* name, int64_t value) {
  // NOTE(nornagon): this loses precision. base::Value can't store int64_t.
  current_.Set(name, static_cast<int>(value));
}

void GPUInfoEnumerator::AddInt(const char* name, int value) {
  current_.Set(name, value);
}

void GPUInfoEnumerator::AddString(const char* name, const std::string& value) {
  if (!value.empty())
    current_.Set(name, value);
}

void GPUInfoEnumerator::AddBool(const char* name, bool value) {
  current_.Set(name, value);
}

void GPUInfoEnumerator::AddTimeDeltaInSecondsF(const char* name,
                                               const base::TimeDelta& value) {
  current_.Set(name, value.InMillisecondsF());
}

void GPUInfoEnumerator::AddBinary(const char* name,
                                  const base::span<const uint8_t>& value) {
  current_.Set(name, base::Value(value));
}

void GPUInfoEnumerator::BeginGPUDevice() {
  value_stack_.push(std::move(current_));
  current_ = {};
}

void GPUInfoEnumerator::EndGPUDevice() {
  auto& top_value = value_stack_.top();
  // GPUDevice can be more than one. So create a list of all.
  // The first one is the active GPU device.
  top_value.EnsureList(kGPUDeviceKey)->Append(std::move(current_));
  current_ = std::move(top_value);
  value_stack_.pop();
}

void GPUInfoEnumerator::BeginVideoDecodeAcceleratorSupportedProfile() {
  value_stack_.push(std::move(current_));
  current_ = {};
}

void GPUInfoEnumerator::EndVideoDecodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack_.top();
  top_value.Set(kVideoDecodeAcceleratorSupportedProfileKey,
                std::move(current_));
  current_ = std::move(top_value);
  value_stack_.pop();
}

void GPUInfoEnumerator::BeginVideoEncodeAcceleratorSupportedProfile() {
  value_stack_.push(std::move(current_));
  current_ = {};
}

void GPUInfoEnumerator::EndVideoEncodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack_.top();
  top_value.Set(kVideoEncodeAcceleratorSupportedProfileKey,
                std::move(current_));
  current_ = std::move(top_value);
  value_stack_.pop();
}

void GPUInfoEnumerator::BeginImageDecodeAcceleratorSupportedProfile() {
  value_stack_.push(std::move(current_));
  current_ = {};
}

void GPUInfoEnumerator::EndImageDecodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack_.top();
  top_value.Set(kImageDecodeAcceleratorSupportedProfileKey,
                std::move(current_));
  current_ = std::move(top_value);
  value_stack_.pop();
}

void GPUInfoEnumerator::BeginAuxAttributes() {
  value_stack_.push(std::move(current_));
  current_ = {};
}

void GPUInfoEnumerator::EndAuxAttributes() {
  auto& top_value = value_stack_.top();
  top_value.Set(kAuxAttributesKey, std::move(current_));
  current_ = std::move(top_value);
  value_stack_.pop();
}

void GPUInfoEnumerator::BeginOverlayInfo() {
  value_stack_.push(std::move(current_));
  current_ = {};
}

void GPUInfoEnumerator::EndOverlayInfo() {
  auto& top_value = value_stack_.top();
  top_value.Set(kOverlayInfo, std::move(current_));
  current_ = std::move(top_value);
  value_stack_.pop();
}

base::Value::Dict GPUInfoEnumerator::GetDictionary() {
  return std::move(current_);
}

}  // namespace electron
