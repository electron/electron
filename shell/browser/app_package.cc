// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/app_package.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "electron/fuses.h"
#include "shell/browser/browser.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/node_bindings.h"
#include "shell/common/thread_restrictions.h"
#include "third_party/icu/source/common/unicode/normalizer2.h"
#include "third_party/icu/source/common/unicode/uchar.h"

#if BUILDFLAG(IS_LINUX)
#include "base/environment.h"
#endif

namespace electron {

namespace {

std::string Trimmed(const std::string& value) {
  std::u16string trimmed;
  base::TrimWhitespace(base::UTF8ToUTF16(value), base::TRIM_ALL, &trimmed);
  return base::UTF16ToUTF8(trimmed);
}

#if BUILDFLAG(IS_WIN)
std::wstring WithoutWhitespace(std::wstring value) {
  std::erase_if(value, [](wchar_t c) { return base::IsAsciiWhitespace(c); });
  return value;
}

// A Squirrel.Windows install is <package>/app-<version>/<exe> next to
// <package>/Update.exe, and Squirrel gives its shortcuts the app user model id
// com.squirrel.<package>.<exe>; use the same one so renderer processes and
// windows group with the shortcut.
void MaybeSetSquirrelAppUserModelId() {
  base::FilePath exe;
  if (!base::PathService::Get(base::FILE_EXE, &exe))
    return;
  const base::FilePath package_dir = exe.DirName().DirName();
  if (!base::PathExists(package_dir.Append(FILE_PATH_LITERAL("update.exe"))))
    return;
  std::wstring exe_name = exe.BaseName().value();
  if (base::EndsWith(exe_name, L".exe", base::CompareCase::INSENSITIVE_ASCII))
    exe_name.resize(exe_name.size() - 4);
  Browser::Get()->SetAppUserModelID(
      L"com.squirrel." + WithoutWhitespace(package_dir.BaseName().value()) +
      L"." + WithoutWhitespace(exe_name));
}
#endif

void Apply(const base::DictValue& manifest) {
  Browser* browser = Browser::Get();
  if (const std::string* version = manifest.FindString("version"))
    browser->SetVersion(*version);
  if (const std::string* product_name = manifest.FindString("productName")) {
    browser->SetName(Trimmed(*product_name));
  } else if (const std::string* name = manifest.FindString("name")) {
    browser->SetName(Trimmed(*name));
  }
#if BUILDFLAG(IS_LINUX)
  const std::string* desktop_name = manifest.FindString("desktopName");
  base::Environment::Create()->SetVar(
      "CHROME_DESKTOP",
      desktop_name && !desktop_name->empty()
          ? *desktop_name
          : DefaultDesktopName(base::UTF8ToUTF16(browser->GetName())));
#endif
}

}  // namespace

std::optional<AppPackage> LoadAppPackage() {
  ScopedAllowBlockingForElectron allow_blocking;
#if BUILDFLAG(IS_WIN)
  MaybeSetSquirrelAppUserModelId();
#endif
  const bool only_asar = fuses::IsOnlyLoadAppFromAsarEnabled();
  std::vector<base::FilePath::StringViewType> candidates;
  if (only_asar) {
    candidates = {FILE_PATH_LITERAL("app.asar")};
  } else {
    candidates = {FILE_PATH_LITERAL("app.asar"), FILE_PATH_LITERAL("app"),
                  FILE_PATH_LITERAL("default_app.asar")};
  }
  const base::FilePath resources = GetResourcesPath();
  for (const auto& candidate : candidates) {
    base::FilePath path = resources.Append(candidate);
    if (only_asar && !asar::GetOrCreateAsarArchive(path))
      continue;
    std::string json;
    if (!asar::ReadFileToString(path.Append(FILE_PATH_LITERAL("package.json")),
                                &json)) {
      continue;
    }
    // As Node reads it: BOM dropped, bad UTF-8 replaced rather than fatal.
    if (!base::IsStringUTF8AllowingNoncharacters(json))
      json = base::UTF16ToUTF8(base::UTF8ToUTF16(json));
    std::string_view json_view(json);
    if (json_view.starts_with("\xEF\xBB\xBF"))
      json_view.remove_prefix(3);
    std::optional<base::Value> manifest = base::JSONReader::Read(
        json_view, base::JSON_REPLACE_INVALID_CHARACTERS);
    if (!manifest || !manifest->is_dict())
      continue;
    const base::DictValue& dict = manifest->GetDict();
    Apply(dict);
    AppPackage package;
    package.path = path;
    const std::string* main = dict.FindString("main");
    package.main = main && !main->empty() ? *main : "index.js";
    if (const std::string* v8_flags = dict.FindString("v8Flags"))
      package.v8_flags = *v8_flags;
    const std::string* type = dict.FindString("type");
    package.esm =
        (type && *type == "module" && !base::EndsWith(package.main, ".cjs")) ||
        base::EndsWith(package.main, ".mjs");
    return package;
  }
  return std::nullopt;
}

std::string DefaultDesktopName(const std::u16string& app_name) {
  // NFKD splits accented characters into base character + combining marks;
  // the marks are dropped, ASCII letters and digits kept in lower case, and
  // every other run of characters becomes one '-'.
  std::string slug;
  UErrorCode status = U_ZERO_ERROR;
  const icu::Normalizer2* nfkd = icu::Normalizer2::getNFKDInstance(status);
  icu::UnicodeString decomposed;
  if (U_SUCCESS(status)) {
    nfkd->normalize(icu::UnicodeString(false, app_name.data(), app_name.size()),
                    decomposed, status);
  }
  if (U_SUCCESS(status)) {
    bool pending_separator = false;
    for (int32_t i = 0; i < decomposed.length();
         i = decomposed.moveIndex32(i, 1)) {
      const UChar32 c = decomposed.char32At(i);
      if (U_GET_GC_MASK(c) & U_GC_M_MASK)
        continue;
      const UChar32 lower = u_tolower(c);
      if ((lower >= 'a' && lower <= 'z') || (lower >= '0' && lower <= '9')) {
        if (pending_separator && !slug.empty())
          slug += '-';
        pending_separator = false;
        slug += static_cast<char>(lower);
      } else {
        pending_separator = true;
      }
    }
  }
  if (slug.empty()) {
    base::FilePath exe;
    base::PathService::Get(base::FILE_EXE, &exe);
    slug = exe.BaseName().AsUTF8Unsafe();
  }
  return slug + ".desktop";
}

}  // namespace electron
