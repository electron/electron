// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/language_util.h"

#include <roapi.h>
#include <windows.system.userprofile.h>
#include <wrl.h>

#include "base/strings/sys_string_conversions.h"
#include "base/win/core_winrt_util.h"
#include "base/win/i18n.h"
#include "base/win/win_util.h"

namespace electron {

bool GetPreferredLanguagesUsingGlobalization(
    std::vector<std::wstring>* languages) {
  base::win::ScopedHString guid = base::win::ScopedHString::Create(
      RuntimeClass_Windows_System_UserProfile_GlobalizationPreferences);
  Microsoft::WRL::ComPtr<
      ABI::Windows::System::UserProfile::IGlobalizationPreferencesStatics>
      prefs;

  HRESULT hr =
      base::win::RoGetActivationFactory(guid.get(), IID_PPV_ARGS(&prefs));
  if (FAILED(hr))
    return false;

  ABI::Windows::Foundation::Collections::IVectorView<HSTRING>* langs;
  hr = prefs->get_Languages(&langs);
  if (FAILED(hr))
    return false;

  unsigned size;
  hr = langs->get_Size(&size);
  if (FAILED(hr))
    return false;

  for (unsigned i = 0; i < size; ++i) {
    HSTRING hstr;
    hr = langs->GetAt(i, &hstr);
    if (SUCCEEDED(hr)) {
      std::wstring_view str = base::win::ScopedHString(hstr).Get();
      languages->emplace_back(str.data(), str.size());
    }
  }

  return true;
}

std::vector<std::string> GetPreferredLanguages() {
  std::vector<std::wstring> languages16;

  // Attempt to use API available on Windows 10 or later, which
  // returns the full list of language preferences.
  if (!GetPreferredLanguagesUsingGlobalization(&languages16)) {
    base::win::i18n::GetThreadPreferredUILanguageList(&languages16);
  }

  std::vector<std::string> languages;
  for (const auto& language : languages16) {
    languages.push_back(base::SysWideToUTF8(language));
  }
  return languages;
}

}  // namespace electron
