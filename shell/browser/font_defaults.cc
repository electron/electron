// Copyright (c) 2018 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/font_defaults.h"

#include <string>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/strings/cstring_view.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/platform_locale_settings.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif

#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
// If a font name in prefs default values starts with a comma, consider it's a
// comma-separated font list and resolve it to the first available font.
#define PREFS_FONT_LIST 1
#include "ui/gfx/font_list.h"
#else
#define PREFS_FONT_LIST 0
#endif

namespace {

#if BUILDFLAG(IS_WIN)
// On Windows with antialiasing we want to use an alternate fixed font like
// Consolas, which looks much better than Courier New.
bool ShouldUseAlternateDefaultFixedFont(const std::string& script) {
  if (!base::StartsWith(script, "courier",
                        base::CompareCase::INSENSITIVE_ASCII)) {
    return false;
  }
  UINT smooth_type = 0;
  SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &smooth_type, 0);
  return smooth_type == FE_FONTSMOOTHINGCLEARTYPE;
}
#endif

// Returns the script of the font pref |pref_name|.  For example, suppose
// |pref_name| is "webkit.webprefs.fonts.serif.Hant".  Since the script code for
// the script name "Hant" is USCRIPT_TRADITIONAL_HAN, the function returns
// USCRIPT_TRADITIONAL_HAN.  |pref_name| must be a valid font pref name.
UScriptCode GetScriptOfFontPref(const base::cstring_view pref_name) {
  // ICU script names are four letters.
  static const size_t kScriptNameLength = 4;

  size_t len = pref_name.size();
  DCHECK_GT(len, kScriptNameLength);
  const char* scriptName = pref_name.substr(len - kScriptNameLength).data();
  int32_t code = u_getPropertyValueEnum(UCHAR_SCRIPT, scriptName);
  DCHECK(code >= 0 && code < USCRIPT_CODE_LIMIT);
  return static_cast<UScriptCode>(code);
}

// Returns the primary script used by the browser's UI locale.  For example, if
// the locale is "ru", the function returns USCRIPT_CYRILLIC, and if the locale
// is "en", the function returns USCRIPT_LATIN.
UScriptCode GetScriptOfBrowserLocale(const std::string& locale) {
  // For Chinese locales, uscript_getCode() just returns USCRIPT_HAN but our
  // per-script fonts are for USCRIPT_SIMPLIFIED_HAN and
  // USCRIPT_TRADITIONAL_HAN.
  if (locale == "zh-CN") {
    return USCRIPT_SIMPLIFIED_HAN;
  }
  if (locale == "zh-TW") {
    return USCRIPT_TRADITIONAL_HAN;
  }
  // For Korean and Japanese, multiple scripts are returned by
  // |uscript_getCode|, but we're passing a one entry buffer leading
  // the buffer to be filled by USCRIPT_INVALID_CODE. We need to
  // hard-code the results for them.
  if (locale == "ko") {
    return USCRIPT_HANGUL;
  }
  if (locale == "ja") {
    return USCRIPT_JAPANESE;
  }

  UScriptCode code = USCRIPT_INVALID_CODE;
  UErrorCode err = U_ZERO_ERROR;
  uscript_getCode(locale.c_str(), &code, 1, &err);

  if (U_FAILURE(err)) {
    code = USCRIPT_INVALID_CODE;
  }
  return code;
}

struct FontDefault {
  const char* pref_name;
  int resource_id;
};

// The following list of font defaults was copied from
// https://chromium.googlesource.com/chromium/src/+/69.0.3497.106/chrome/browser/ui/prefs/prefs_tab_helper.cc#152
//
// The only updates that should be made to this list are copying updates that
// were made in Chromium.
//
// vvvvv DO NOT EDIT vvvvv

// Font pref defaults.  The prefs that have defaults vary by platform, since not
// all platforms have fonts for all scripts for all generic families.
// TODO(falken): add proper defaults when possible for all
// platforms/scripts/generic families.
constexpr auto kFontDefaults = std::to_array<FontDefault>({
    {prefs::kWebKitStandardFontFamily, IDS_STANDARD_FONT_FAMILY},
    {prefs::kWebKitFixedFontFamily, IDS_FIXED_FONT_FAMILY},
    {prefs::kWebKitSerifFontFamily, IDS_SERIF_FONT_FAMILY},
    {prefs::kWebKitSansSerifFontFamily, IDS_SANS_SERIF_FONT_FAMILY},
    {prefs::kWebKitCursiveFontFamily, IDS_CURSIVE_FONT_FAMILY},
    {prefs::kWebKitFantasyFontFamily, IDS_FANTASY_FONT_FAMILY},
    {prefs::kWebKitMathFontFamily, IDS_MATH_FONT_FAMILY},
#if BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
    {prefs::kWebKitStandardFontFamilyJapanese,
     IDS_STANDARD_FONT_FAMILY_JAPANESE},
    {prefs::kWebKitFixedFontFamilyJapanese, IDS_FIXED_FONT_FAMILY_JAPANESE},
    {prefs::kWebKitSerifFontFamilyJapanese, IDS_SERIF_FONT_FAMILY_JAPANESE},
    {prefs::kWebKitSansSerifFontFamilyJapanese,
     IDS_SANS_SERIF_FONT_FAMILY_JAPANESE},
    {prefs::kWebKitStandardFontFamilyKorean, IDS_STANDARD_FONT_FAMILY_KOREAN},
    {prefs::kWebKitSerifFontFamilyKorean, IDS_SERIF_FONT_FAMILY_KOREAN},
    {prefs::kWebKitSansSerifFontFamilyKorean,
     IDS_SANS_SERIF_FONT_FAMILY_KOREAN},
    {prefs::kWebKitStandardFontFamilySimplifiedHan,
     IDS_STANDARD_FONT_FAMILY_SIMPLIFIED_HAN},
    {prefs::kWebKitSerifFontFamilySimplifiedHan,
     IDS_SERIF_FONT_FAMILY_SIMPLIFIED_HAN},
    {prefs::kWebKitSansSerifFontFamilySimplifiedHan,
     IDS_SANS_SERIF_FONT_FAMILY_SIMPLIFIED_HAN},
    {prefs::kWebKitStandardFontFamilyTraditionalHan,
     IDS_STANDARD_FONT_FAMILY_TRADITIONAL_HAN},
    {prefs::kWebKitSerifFontFamilyTraditionalHan,
     IDS_SERIF_FONT_FAMILY_TRADITIONAL_HAN},
    {prefs::kWebKitSansSerifFontFamilyTraditionalHan,
     IDS_SANS_SERIF_FONT_FAMILY_TRADITIONAL_HAN},
#endif
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
    {prefs::kWebKitCursiveFontFamilySimplifiedHan,
     IDS_CURSIVE_FONT_FAMILY_SIMPLIFIED_HAN},
    {prefs::kWebKitCursiveFontFamilyTraditionalHan,
     IDS_CURSIVE_FONT_FAMILY_TRADITIONAL_HAN},
#endif
#if BUILDFLAG(IS_CHROMEOS)
    {prefs::kWebKitStandardFontFamilyArabic, IDS_STANDARD_FONT_FAMILY_ARABIC},
    {prefs::kWebKitSerifFontFamilyArabic, IDS_SERIF_FONT_FAMILY_ARABIC},
    {prefs::kWebKitSansSerifFontFamilyArabic,
     IDS_SANS_SERIF_FONT_FAMILY_ARABIC},
    {prefs::kWebKitFixedFontFamilyKorean, IDS_FIXED_FONT_FAMILY_KOREAN},
    {prefs::kWebKitFixedFontFamilySimplifiedHan,
     IDS_FIXED_FONT_FAMILY_SIMPLIFIED_HAN},
    {prefs::kWebKitFixedFontFamilyTraditionalHan,
     IDS_FIXED_FONT_FAMILY_TRADITIONAL_HAN},
#elif BUILDFLAG(IS_WIN)
    {prefs::kWebKitFixedFontFamilyArabic, IDS_FIXED_FONT_FAMILY_ARABIC},
    {prefs::kWebKitSansSerifFontFamilyArabic,
     IDS_SANS_SERIF_FONT_FAMILY_ARABIC},
    {prefs::kWebKitStandardFontFamilyCyrillic,
     IDS_STANDARD_FONT_FAMILY_CYRILLIC},
    {prefs::kWebKitFixedFontFamilyCyrillic, IDS_FIXED_FONT_FAMILY_CYRILLIC},
    {prefs::kWebKitSerifFontFamilyCyrillic, IDS_SERIF_FONT_FAMILY_CYRILLIC},
    {prefs::kWebKitSansSerifFontFamilyCyrillic,
     IDS_SANS_SERIF_FONT_FAMILY_CYRILLIC},
    {prefs::kWebKitStandardFontFamilyGreek, IDS_STANDARD_FONT_FAMILY_GREEK},
    {prefs::kWebKitFixedFontFamilyGreek, IDS_FIXED_FONT_FAMILY_GREEK},
    {prefs::kWebKitSerifFontFamilyGreek, IDS_SERIF_FONT_FAMILY_GREEK},
    {prefs::kWebKitSansSerifFontFamilyGreek, IDS_SANS_SERIF_FONT_FAMILY_GREEK},
    {prefs::kWebKitFixedFontFamilyKorean, IDS_FIXED_FONT_FAMILY_KOREAN},
    {prefs::kWebKitCursiveFontFamilyKorean, IDS_CURSIVE_FONT_FAMILY_KOREAN},
    {prefs::kWebKitFixedFontFamilySimplifiedHan,
     IDS_FIXED_FONT_FAMILY_SIMPLIFIED_HAN},
    {prefs::kWebKitFixedFontFamilyTraditionalHan,
     IDS_FIXED_FONT_FAMILY_TRADITIONAL_HAN},
#endif
});

// ^^^^^ DO NOT EDIT ^^^^^

auto MakeDefaultFontCopier() {
  using namespace prefs;
  using WP = blink::web_pref::WebPreferences;
  using FamilyMap = blink::web_pref::ScriptFontFamilyMap;

  // Map from a family name (e.g. "webkit.webprefs.fonts.fixed") to a
  // Pointer-to-Member of the location in WebPreferences of its
  // ScriptFontFamilyMap (e.g. &WebPreferences::fixed_font_family_map)
  static constexpr auto FamilyMapByName =
      base::MakeFixedFlatMap<std::string_view, FamilyMap WP::*>({
          {kWebKitStandardFontFamilyMap, &WP::standard_font_family_map},
          {kWebKitFixedFontFamilyMap, &WP::fixed_font_family_map},
          {kWebKitSerifFontFamilyMap, &WP::serif_font_family_map},
          {kWebKitSansSerifFontFamilyMap, &WP::sans_serif_font_family_map},
          {kWebKitCursiveFontFamilyMap, &WP::cursive_font_family_map},
      });

  WP defaults;

  std::set<std::string> fonts_with_defaults;
  UScriptCode browser_script =
      GetScriptOfBrowserLocale(g_browser_process->GetApplicationLocale());
  // Populate `defaults`'s ScriptFontFamilyMaps with the values from
  // the kFontDefaults array in the "DO NOT EDIT" section of this file.
  //
  // The kFontDefaults's `pref_name` field is built as `${family}.${script}`,
  // so splitting on the last '.' gives the family and script: a pref key of
  // "webkit.webprefs.fonts.fixed.Zyyy" splits into family name
  // "webkit.webprefs.fonts.fixed" and script "Zyyy". (Yes, "Zyyy" is real.
  // See pref_font_script_names-inl.h for the full list :)
  for (auto [pref_name, resource_id] : kFontDefaults) {
#if BUILDFLAG(IS_WIN)
    if (pref_name == prefs::kWebKitFixedFontFamily) {
      if (ShouldUseAlternateDefaultFixedFont(
              l10n_util::GetStringUTF8(resource_id))) {
        resource_id = IDS_FIXED_FONT_FAMILY_ALT_WIN;
      }
    }
#endif
    UScriptCode pref_script =
        GetScriptOfFontPref(UNSAFE_BUFFERS(base::cstring_view(pref_name)));
    // Suppress this default font pref value if it is for the primary script of
    // the browser's UI locale.  For example, if the pref is for the sans-serif
    // font for the Cyrillic script, and the browser locale is "ru" (Russian),
    // the default is suppressed.  Otherwise, the default would override the
    // user's font preferences when viewing pages in their native language.
    // This is because users have no way yet of customizing their per-script
    // font preferences.  The font prefs accessible in the options UI are for
    // the default, unknown script; these prefs have less priority than the
    // per-script font prefs when the script of the content is known.  This code
    // can possibly be removed later if users can easily access per-script font
    // prefs (e.g., via the extensions workflow), or the problem turns out to
    // not be really critical after all.
    if (browser_script != pref_script) {
      std::string value = l10n_util::GetStringUTF8(resource_id);
#if PREFS_FONT_LIST
      if (value.starts_with(',')) {
        value = gfx::FontList::FirstAvailableOrFirst(value);
      }
#else   // !PREFS_FONT_LIST
      DCHECK(!value.starts_with(','))
          << "This platform doesn't support default font lists. " << pref_name
          << "=" << value;
#endif  // PREFS_FONT_LIST
      const auto [family, script] = *base::RSplitStringOnce(pref_name, '.');
      if (auto* family_map_ptr = base::FindOrNull(FamilyMapByName, family)) {
        FamilyMap& family_map = defaults.**family_map_ptr;
        family_map[std::string{script}] = base::UTF8ToUTF16(value);
      }
      fonts_with_defaults.insert(pref_name);
    }
  }

  // Expand the font concatenated with script name so this stays at RO memory
  // rather than allocated in heap.
  // clang-format off
  static const auto kFontFamilyMap = std::to_array<const char *>({
    #define EXPAND_SCRIPT_FONT(map_name, script_name) map_name "." script_name,

    #include "chrome/common/pref_font_script_names-inl.h"
    ALL_FONT_SCRIPTS(WEBKIT_WEBPREFS_FONTS_CURSIVE)
    ALL_FONT_SCRIPTS(WEBKIT_WEBPREFS_FONTS_FIXED)
    ALL_FONT_SCRIPTS(WEBKIT_WEBPREFS_FONTS_SANSERIF)
    ALL_FONT_SCRIPTS(WEBKIT_WEBPREFS_FONTS_SERIF)
    ALL_FONT_SCRIPTS(WEBKIT_WEBPREFS_FONTS_STANDARD)

    #undef EXPAND_SCRIPT_FONT
  });
  // clang-format on

  for (const char* const pref_name : kFontFamilyMap) {
    if (fonts_with_defaults.find(pref_name) == fonts_with_defaults.end()) {
      // We haven't already set a default value for this font preference, so set
      // an empty string as the default.
      const auto [family, script] = *base::RSplitStringOnce(pref_name, '.');
      if (auto* family_map_ptr = base::FindOrNull(FamilyMapByName, family)) {
        FamilyMap& family_map = defaults.**family_map_ptr;
        family_map[std::string{script}] = std::u16string();
      }
    }
  }

  // Lambda that copies all of `default`'s fonts into `prefs`
  auto copy_default_fonts_to_web_prefs = [defaults](WP* prefs) {
    for (const auto [_, family_map_ptr] : FamilyMapByName) {
      const FamilyMap& src = defaults.*family_map_ptr;
      FamilyMap& tgt = prefs->*family_map_ptr;
      for (const auto& [key, val] : src)
        tgt[key] = val;
    }
  };

  return copy_default_fonts_to_web_prefs;
}
}  // namespace

namespace electron {

void SetFontDefaults(blink::web_pref::WebPreferences* prefs) {
  static const auto& copy_default_fonts_to_web_prefs = MakeDefaultFontCopier();
  copy_default_fonts_to_web_prefs(prefs);
}

}  // namespace electron
