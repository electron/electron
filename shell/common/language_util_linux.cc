// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/language_util.h"

#include <glib.h>

#include "base/check.h"
#include "base/i18n/rtl.h"

namespace electron {

std::vector<std::string> GetPreferredLanguages() {
  std::vector<std::string> preferredLanguages;

  // Based on
  // https://source.chromium.org/chromium/chromium/src/+/refs/tags/108.0.5329.0:ui/base/l10n/l10n_util.cc;l=543-554
  // GLib implements correct environment variable parsing with
  // the precedence order: LANGUAGE, LC_ALL, LC_MESSAGES and LANG.
  const char* const* languages = g_get_language_names();
  DCHECK(languages);   // A valid pointer is guaranteed.
  DCHECK(*languages);  // At least one entry, "C", is guaranteed.

  // SAFETY: |g_get_language_names()| returns a glib-owned array
  // of const C strings, terminated by an empty string.
  // This loop is the correct way to walk through its return values.
  static constexpr std::string_view CLangName = "C";
  for (; *languages; UNSAFE_BUFFERS(++languages)) {
    if (CLangName != *languages)
      preferredLanguages.push_back(base::i18n::GetCanonicalLocale(*languages));
  }

  return preferredLanguages;
}

}  // namespace electron
