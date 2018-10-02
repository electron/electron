// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_GPU_INFO_ENUMERATOR_H_
#define ATOM_BROWSER_API_GPU_INFO_ENUMERATOR_H_

#include <memory>
#include <stack>
#include <string>

#include "base/values.h"
#include "gpu/config/gpu_info.h"

namespace atom {

// This class implements the enumerator for reading all the attributes in
// GPUInfo into a dictionary.
class GPUInfoEnumerator final : public gpu::GPUInfo::Enumerator {
  const char* kGPUDeviceKey = "gpuDevice";
  const char* kVideoDecodeAcceleratorSupportedProfileKey =
      "videoDecodeAcceleratorSupportedProfile";
  const char* kVideoEncodeAcceleratorSupportedProfileKey =
      "videoEncodeAcceleratorSupportedProfile";
  const char* kAuxAttributesKey = "auxAttributes";
  const char* kOverlayCapabilityKey = "overlayCapability";

 public:
  GPUInfoEnumerator();
  ~GPUInfoEnumerator() override;
  void AddInt64(const char* name, int64_t value) override;
  void AddInt(const char* name, int value) override;
  void AddString(const char* name, const std::string& value) override;
  void AddBool(const char* name, bool value) override;
  void AddTimeDeltaInSecondsF(const char* name,
                              const base::TimeDelta& value) override;
  void BeginGPUDevice() override;
  void EndGPUDevice() override;
  void BeginVideoDecodeAcceleratorSupportedProfile() override;
  void EndVideoDecodeAcceleratorSupportedProfile() override;
  void BeginVideoEncodeAcceleratorSupportedProfile() override;
  void EndVideoEncodeAcceleratorSupportedProfile() override;
  void BeginAuxAttributes() override;
  void EndAuxAttributes() override;
  void BeginOverlayCapability() override;
  void EndOverlayCapability() override;
  std::unique_ptr<base::DictionaryValue> GetDictionary();

 private:
  // The stack is used to manage nested values
  std::stack<std::unique_ptr<base::DictionaryValue>> value_stack;
  std::unique_ptr<base::DictionaryValue> current;
};

}  // namespace atom
#endif  // ATOM_BROWSER_API_GPU_INFO_ENUMERATOR_H_
