// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/gpuinfo_manager.h"
#include "atom/browser/api/gpu_info_enumerator.h"
#include "base/memory/singleton.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/config/gpu_info_collector.h"

namespace atom {

GPUInfoManager* GPUInfoManager::GetInstance() {
  return base::Singleton<GPUInfoManager>::get();
}

GPUInfoManager::GPUInfoManager()
    : gpu_data_manager_(content::GpuDataManagerImpl::GetInstance()) {
  gpu_data_manager_->AddObserver(this);
}

GPUInfoManager::~GPUInfoManager() {
  content::GpuDataManagerImpl::GetInstance()->RemoveObserver(this);
}

// Based on
// https://chromium.googlesource.com/chromium/src.git/+/69.0.3497.106/content/browser/gpu/gpu_data_manager_impl_private.cc#810
bool GPUInfoManager::NeedsCompleteGpuInfoCollection() {
#if defined(OS_WIN)
  const auto& gpu_info = gpu_data_manager_->GetGPUInfo();
  return (gpu_info.dx_diagnostics.values.empty() &&
          gpu_info.dx_diagnostics.children.empty());
#else
  return false;
#endif
}

// Should be posted to the task runner
void GPUInfoManager::ProcessCompleteInfo() {
  const auto result = EnumerateGPUInfo(gpu_data_manager_->GetGPUInfo());
  // We have received the complete information, resolve all promises that
  // were waiting for this info.
  for (const auto& promise : complete_info_promise_set_) {
    promise->Resolve(*result);
  }
  complete_info_promise_set_.clear();
}

void GPUInfoManager::OnGpuInfoUpdate() {
  // Ignore if called when not asked for complete GPUInfo
  if (NeedsCompleteGpuInfoCollection())
    return;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&GPUInfoManager::ProcessCompleteInfo,
                                base::Unretained(this)));
}

// Should be posted to the task runner
void GPUInfoManager::CompleteInfoFetcher(scoped_refptr<util::Promise> promise) {
  complete_info_promise_set_.push_back(promise);

  if (NeedsCompleteGpuInfoCollection()) {
    gpu_data_manager_->RequestCompleteGpuInfoIfNeeded();
  } else {
    GPUInfoManager::OnGpuInfoUpdate();
  }
}

void GPUInfoManager::FetchCompleteInfo(scoped_refptr<util::Promise> promise) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&GPUInfoManager::CompleteInfoFetcher,
                                base::Unretained(this), promise));
}

// This fetches the info synchronously, so no need to post to the task queue.
// There cannot be multiple promises as they are resolved synchronously.
void GPUInfoManager::FetchBasicInfo(scoped_refptr<util::Promise> promise) {
  gpu::GPUInfo gpu_info;
  CollectBasicGraphicsInfo(&gpu_info);
  promise->Resolve(*EnumerateGPUInfo(gpu_info));
}

std::unique_ptr<base::DictionaryValue> GPUInfoManager::EnumerateGPUInfo(
    gpu::GPUInfo gpu_info) const {
  GPUInfoEnumerator enumerator;
  gpu_info.EnumerateFields(&enumerator);
  return enumerator.GetDictionary();
}

}  // namespace atom
