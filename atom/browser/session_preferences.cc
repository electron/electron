// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/session_preferences.h"

#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"

namespace atom {

namespace {

#if defined(OS_WIN)
const base::FilePath::CharType kPathDelimiter = FILE_PATH_LITERAL(';');
#else
const base::FilePath::CharType kPathDelimiter = FILE_PATH_LITERAL(':');
#endif

}  // namespace

// static
int SessionPreferences::kLocatorKey = 0;

SessionPreferences::SessionPreferences(content::BrowserContext* context) {
  context->SetUserData(&kLocatorKey, base::WrapUnique(this));
}

SessionPreferences::~SessionPreferences() {}

// static
SessionPreferences* SessionPreferences::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SessionPreferences*>(context->GetUserData(&kLocatorKey));
}

// static
void SessionPreferences::AppendExtraCommandLineSwitches(
    content::BrowserContext* context,
    base::CommandLine* command_line) {
  SessionPreferences* self = FromBrowserContext(context);
  if (!self)
    return;

  base::FilePath::StringType preloads;
  for (const auto& preload : self->preloads()) {
    if (!base::FilePath(preload).IsAbsolute()) {
      LOG(ERROR) << "preload script must have absolute path: " << preload;
      continue;
    }
    if (preloads.empty())
      preloads = preload;
    else
      preloads += kPathDelimiter + preload;
  }
  if (!preloads.empty())
    command_line->AppendSwitchNative(switches::kPreloadScripts, preloads);
}

}  // namespace atom
