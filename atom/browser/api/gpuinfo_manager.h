// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_GPUINFO_MANAGER_H_
#define ATOM_BROWSER_API_GPUINFO_MANAGER_H_

#include <memory>
#include <unordered_set>
#include <vector>

#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/promise_util.h"
#include "content/browser/gpu/gpu_data_manager_impl.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/gpu_data_manager_observer.h"

namespace atom {

// GPUInfoManager is a singleton used to manage and fetch GPUInfo
class GPUInfoManager : public content::GpuDataManagerObserver {
 public:
  static GPUInfoManager* GetInstance();

  GPUInfoManager();
  ~GPUInfoManager() override;
  bool NeedsCompleteGpuInfoCollection() const;
  void FetchCompleteInfo(scoped_refptr<util::Promise> promise);
  void FetchBasicInfo(scoped_refptr<util::Promise> promise);
  void OnGpuInfoUpdate() override;

 private:
  std::unique_ptr<base::DictionaryValue> EnumerateGPUInfo(
      gpu::GPUInfo gpu_info) const;

  // These should be posted to the task queue
  void CompleteInfoFetcher(scoped_refptr<util::Promise> promise);
  void ProcessCompleteInfo();

  // This set maintains all the promises that should be fulfilled
  // once we have the complete information data
  std::vector<scoped_refptr<util::Promise>> complete_info_promise_set_;
  content::GpuDataManager* gpu_data_manager_;

  DISALLOW_COPY_AND_ASSIGN(GPUInfoManager);
};

}  // namespace atom
#endif  // ATOM_BROWSER_API_GPUINFO_MANAGER_H_
