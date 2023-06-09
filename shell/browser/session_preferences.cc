// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/session_preferences.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_context.h"

namespace electron {

// static
int SessionPreferences::kLocatorKey = 0;

// static
void SessionPreferences::CreateForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  context->SetUserData(&kLocatorKey,
                       base::WrapUnique(new SessionPreferences{}));
}

SessionPreferences::SessionPreferences() = default;
SessionPreferences::~SessionPreferences() = default;

// static
SessionPreferences* SessionPreferences::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SessionPreferences*>(context->GetUserData(&kLocatorKey));
}

// static
std::vector<base::FilePath> SessionPreferences::GetValidPreloads(
    content::BrowserContext* context) {
  std::vector<base::FilePath> result;

  if (auto* self = FromBrowserContext(context)) {
    for (const auto& preload : self->preloads()) {
      if (preload.IsAbsolute()) {
        result.emplace_back(preload);
      } else {
        LOG(ERROR) << "preload script must have absolute path: " << preload;
      }
    }
  }

  return result;
}

}  // namespace electron
