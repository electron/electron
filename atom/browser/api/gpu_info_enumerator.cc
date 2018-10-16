// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/gpu_info_enumerator.h"

#include <utility>

namespace atom {

GPUInfoEnumerator::GPUInfoEnumerator()
    : value_stack(), current(std::make_unique<base::DictionaryValue>()) {}

GPUInfoEnumerator::~GPUInfoEnumerator() {}

void GPUInfoEnumerator::AddInt64(const char* name, int64_t value) {
  current->SetInteger(name, value);
}

void GPUInfoEnumerator::AddInt(const char* name, int value) {
  current->SetInteger(name, value);
}

void GPUInfoEnumerator::AddString(const char* name, const std::string& value) {
  if (!value.empty())
    current->SetString(name, value);
}

void GPUInfoEnumerator::AddBool(const char* name, bool value) {
  current->SetBoolean(name, value);
}

void GPUInfoEnumerator::AddTimeDeltaInSecondsF(const char* name,
                                               const base::TimeDelta& value) {
  current->SetInteger(name, value.InMilliseconds());
}

void GPUInfoEnumerator::BeginGPUDevice() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndGPUDevice() {
  auto& top_value = value_stack.top();
  // GPUDevice can be more than one. So create a list of all.
  // The first one is the active GPU device.
  if (top_value->HasKey(kGPUDeviceKey)) {
    base::ListValue* list;
    top_value->GetList(kGPUDeviceKey, &list);
    list->Append(std::move(current));
  } else {
    auto gpus = std::make_unique<base::ListValue>();
    gpus->Append(std::move(current));
    top_value->SetList(kGPUDeviceKey, std::move(gpus));
  }
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginVideoDecodeAcceleratorSupportedProfile() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndVideoDecodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kVideoDecodeAcceleratorSupportedProfileKey,
                           std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginVideoEncodeAcceleratorSupportedProfile() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndVideoEncodeAcceleratorSupportedProfile() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kVideoEncodeAcceleratorSupportedProfileKey,
                           std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginAuxAttributes() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndAuxAttributes() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kAuxAttributesKey, std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

void GPUInfoEnumerator::BeginOverlayCapability() {
  value_stack.push(std::move(current));
  current = std::make_unique<base::DictionaryValue>();
}

void GPUInfoEnumerator::EndOverlayCapability() {
  auto& top_value = value_stack.top();
  top_value->SetDictionary(kOverlayCapabilityKey, std::move(current));
  current = std::move(top_value);
  value_stack.pop();
}

std::unique_ptr<base::DictionaryValue> GPUInfoEnumerator::GetDictionary() {
  return std::move(current);
}

}  // namespace atom
