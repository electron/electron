// Copyright (c) 2018 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/font_defaults.h"

#include <array>
#include <string>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/platform_locale_settings.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// The following list of font defaults was copied from
// https://chromium.googlesource.com/chromium/src/+/69.0.3497.106/chrome/browser/ui/prefs/prefs_tab_helper.cc#152
//
// The only updates that should be made to this list are copying updates that
// were made in Chromium.
//
// vvvvv DO NOT EDIT vvvvv

struct FontDefault {
  const char* pref_name;
  int resource_id;
};

// Font pref defaults.  The prefs that have defaults vary by platform, since not
// all platforms have fonts for all scripts for all generic families.
// TODO(falken): add proper defaults when possible for all
// platforms/scripts/generic families.
const FontDefault kFontDefaults[] = {
    {prefs::kWebKitStandardFontFamily, IDS_STANDARD_FONT_FAMILY},
    {prefs::kWebKitFixedFontFamily, IDS_FIXED_FONT_FAMILY},
    {prefs::kWebKitSerifFontFamily, IDS_SERIF_FONT_FAMILY},
    {prefs::kWebKitSansSerifFontFamily, IDS_SANS_SERIF_FONT_FAMILY},
    {prefs::kWebKitCursiveFontFamily, IDS_CURSIVE_FONT_FAMILY},
    {prefs::kWebKitFantasyFontFamily, IDS_FANTASY_FONT_FAMILY},
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
};

// ^^^^^ DO NOT EDIT ^^^^^

static auto MakeDefaultFontCache() {
  using WP = blink::web_pref::WebPreferences;
  using FamilyMap = blink::web_pref::ScriptFontFamilyMap;

  static constexpr auto FamilyMapByName =
      base::MakeFixedFlatMap<std::string_view, FamilyMap WP::*>({
          {prefs::kWebKitStandardFontFamilyMap, &WP::standard_font_family_map},
          {prefs::kWebKitFixedFontFamilyMap, &WP::fixed_font_family_map},
          {prefs::kWebKitSerifFontFamilyMap, &WP::serif_font_family_map},
          {prefs::kWebKitSansSerifFontFamilyMap,
           &WP::sans_serif_font_family_map},
          {prefs::kWebKitCursiveFontFamilyMap, &WP::cursive_font_family_map},
      });

  WP defaults;
  for (const auto& [pref_name, resource_id] : kFontDefaults) {
    LOG(INFO) << "pref_name [" << pref_name << ']';
    const auto tokens = base::RSplitStringOnce(pref_name, ".");
    if (!tokens)
      continue;
    const auto [family_name, script_name] = *tokens;
    LOG(INFO) << "family_name [" << family_name << "] script_name ["
              << script_name << "] resource_id [" << resource_id << ']';
    if (auto* family_map_ptr = base::FindOrNull(FamilyMapByName, family_name)) {
      (defaults.**family_map_ptr)
          .insert_or_assign(
              std::string{script_name},
              base::UTF8ToUTF16(l10n_util::GetStringUTF8(resource_id)));
    }
  }

  // lambda function that copies the defaults from `cache` into `prefs`
  auto apply_defaults_to_wp = [defaults](WP* prefs) {
    LOG(INFO) << "begin";
    for (const auto [family_name, family_map_ptr] : FamilyMapByName) {
      const auto& src = defaults.*family_map_ptr;
      auto& tgt = prefs->*family_map_ptr;
      for (const auto& [key, default_val] : src) {
        auto& cur_val = tgt[key];
        LOG(INFO) << " family [ " << family_name << "] key [" << key
                  << "] cur_val [" << base::UTF16ToUTF8(cur_val)
                  << "] default_val [" << base::UTF16ToUTF8(default_val) << "]";
        if (cur_val.empty()) {
          cur_val = default_val;
        }
      }
    }
    LOG(INFO) << "end";
  };

  return apply_defaults_to_wp;
}
}  // namespace

namespace electron {

void SetFontDefaults(blink::web_pref::WebPreferences* prefs) {
  static const auto apply_defaults_to_web_prefs = MakeDefaultFontCache();
  apply_defaults_to_web_prefs(prefs);
}

}  // namespace electron
