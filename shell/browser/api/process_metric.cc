// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/process_metric.h"

#include <memory>
#include <utility>

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(OS_MACOSX)
extern "C" int sandbox_check(pid_t pid, const char* operation, int type, ...);
#endif

namespace electron {

ProcessMetric::ProcessMetric(int type,
                             base::ProcessHandle handle,
                             std::unique_ptr<base::ProcessMetrics> metrics) {
  this->type = type;
  this->metrics = std::move(metrics);

#if defined(OS_WIN)
  HANDLE duplicate_handle = INVALID_HANDLE_VALUE;
  ::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(),
                    &duplicate_handle, 0, false, DUPLICATE_SAME_ACCESS);
  this->process = base::Process(duplicate_handle);
#else
  this->process = base::Process(handle);
#endif
}

ProcessMetric::~ProcessMetric() = default;

#if defined(OS_WIN)

ProcessIntegrityLevel ProcessMetric::GetIntegrityLevel() const {
  HANDLE token = nullptr;
  if (!::OpenProcessToken(process.Handle(), TOKEN_QUERY, &token)) {
    return ProcessIntegrityLevel::Unknown;
  }

  base::win::ScopedHandle token_scoped(token);

  DWORD token_info_length = 0;
  if (::GetTokenInformation(token, TokenIntegrityLevel, nullptr, 0,
                            &token_info_length) ||
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return ProcessIntegrityLevel::Unknown;
  }

  auto token_label_bytes = std::make_unique<char[]>(token_info_length);
  TOKEN_MANDATORY_LABEL* token_label =
      reinterpret_cast<TOKEN_MANDATORY_LABEL*>(token_label_bytes.get());
  if (!::GetTokenInformation(token, TokenIntegrityLevel, token_label,
                             token_info_length, &token_info_length)) {
    return ProcessIntegrityLevel::Unknown;
  }

  DWORD integrity_level = *::GetSidSubAuthority(
      token_label->Label.Sid,
      static_cast<DWORD>(*::GetSidSubAuthorityCount(token_label->Label.Sid) -
                         1));

  if (integrity_level >= SECURITY_MANDATORY_UNTRUSTED_RID &&
      integrity_level < SECURITY_MANDATORY_LOW_RID) {
    return ProcessIntegrityLevel::Untrusted;
  }

  if (integrity_level >= SECURITY_MANDATORY_LOW_RID &&
      integrity_level < SECURITY_MANDATORY_MEDIUM_RID) {
    return ProcessIntegrityLevel::Low;
  }

  if (integrity_level >= SECURITY_MANDATORY_MEDIUM_RID &&
      integrity_level < SECURITY_MANDATORY_HIGH_RID) {
    return ProcessIntegrityLevel::Medium;
  }

  if (integrity_level >= SECURITY_MANDATORY_HIGH_RID &&
      integrity_level < SECURITY_MANDATORY_SYSTEM_RID) {
    return ProcessIntegrityLevel::High;
  }

  return ProcessIntegrityLevel::Unknown;
}

// static
bool ProcessMetric::IsSandboxed(ProcessIntegrityLevel integrity_level) {
  return integrity_level > ProcessIntegrityLevel::Unknown &&
         integrity_level < ProcessIntegrityLevel::Medium;
}

#elif defined(OS_MACOSX)

bool ProcessMetric::IsSandboxed() const {
#if defined(MAS_BUILD)
  return true;
#else
  return sandbox_check(process.Pid(), nullptr, 0) != 0;
#endif
}

#endif  // defined(OS_MACOSX)

}  // namespace electron
