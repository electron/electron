// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_PROCESS_METRIC_H_
#define SHELL_BROWSER_API_PROCESS_METRIC_H_

#include <memory>

#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics.h"

namespace electron {

#if defined(OS_WIN)
enum class ProcessIntegrityLevel {
  Unknown,
  Untrusted,
  Low,
  Medium,
  High,
};
#endif

struct ProcessMetric {
  int type;
  base::Process process;
  std::unique_ptr<base::ProcessMetrics> metrics;

  ProcessMetric(int type,
                base::ProcessHandle handle,
                std::unique_ptr<base::ProcessMetrics> metrics);
  ~ProcessMetric();

#if defined(OS_WIN)
  ProcessIntegrityLevel GetIntegrityLevel() const;
  static bool IsSandboxed(ProcessIntegrityLevel integrity_level);
#elif defined(OS_MACOSX)
  bool IsSandboxed() const;
#endif
};

}  // namespace electron

#endif  // SHELL_BROWSER_API_PROCESS_METRIC_H_
