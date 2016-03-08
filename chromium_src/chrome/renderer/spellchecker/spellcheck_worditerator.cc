// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implements a custom word iterator used for our spellchecker.

#include "chrome/renderer/spellchecker/spellcheck_worditerator.h"

#include <map>
#include <string>
#include <utility>

#include "base/i18n/break_iterator.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/renderer/spellchecker/spellcheck.h"
#include "third_party/icu/source/common/unicode/normlzr.h"
#include "third_party/icu/source/common/unicode/schriter.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "third_party/icu/source/i18n/unicode/ulocdata.h"

using base::i18n::BreakIterator;

// SpellcheckCharAttribute implementation:

SpellcheckCharAttribute::SpellcheckCharAttribute()
    : script_code_(USCRIPT_LATIN) {
}

SpellcheckCharAttribute::~SpellcheckCharAttribute() {
}

void SpellcheckCharAttribute::SetDefaultLanguage(const std::string& language) {
  CreateRuleSets(language);
}

base::string16 SpellcheckCharAttribute::GetRuleSet(
    bool allow_contraction) const {
  return allow_contraction ?
      ruleset_allow_contraction_ : ruleset_disallow_contraction_;
}

void SpellcheckCharAttribute::CreateRuleSets(const std::string& language) {
  // The template for our custom rule sets, which is based on the word-break
  // rules of ICU 4.0:
  // <http://source.icu-project.org/repos/icu/icu/tags/release-4-0/source/data/brkitr/word.txt>.
  // The major differences from the original one are listed below:
  // * It discards comments in the original rules.
  // * It discards characters not needed by our spellchecker (e.g. numbers,
  //   punctuation characters, Hiraganas, Katakanas, CJK Ideographs, and so on).
  // * It allows customization of the $ALetter value (i.e. word characters).
  // * It allows customization of the $ALetterPlus value (i.e. whether or not to
  //   use the dictionary data).
  // * It allows choosing whether or not to split a text at contraction
  //   characters.
  // This template only changes the forward-iteration rules. So, calling
  // ubrk_prev() returns the same results as the original template.
  static const char kRuleTemplate[] =
      "!!chain;"
      "$CR           = [\\p{Word_Break = CR}];"
      "$LF           = [\\p{Word_Break = LF}];"
      "$Newline      = [\\p{Word_Break = Newline}];"
      "$Extend       = [\\p{Word_Break = Extend}];"
      "$Format       = [\\p{Word_Break = Format}];"
      "$Katakana     = [\\p{Word_Break = Katakana}];"
      // Not all the characters in a given script are ALetter.
      // For instance, U+05F4 is MidLetter. So, this may be
      // better, but it leads to an empty set error in Thai.
      // "$ALetter   = [[\\p{script=%s}] & [\\p{Word_Break = ALetter}]];"
      "$ALetter      = [\\p{script=%s}%s];"
      // U+0027 (single quote/apostrophe) is not in MidNumLet any more
      // in UAX 29 rev 21 or later. For our purpose, U+0027
      // has to be treated as MidNumLet. ( http://crbug.com/364072 )
      "$MidNumLet    = [\\p{Word_Break = MidNumLet} \\u0027];"
      "$MidLetter    = [\\p{Word_Break = MidLetter}%s];"
      "$MidNum       = [\\p{Word_Break = MidNum}];"
      "$Numeric      = [\\p{Word_Break = Numeric}];"
      "$ExtendNumLet = [\\p{Word_Break = ExtendNumLet}];"

      "$Control        = [\\p{Grapheme_Cluster_Break = Control}]; "
      "%s"  // ALetterPlus

      "$KatakanaEx     = $Katakana     ($Extend |  $Format)*;"
      "$ALetterEx      = $ALetterPlus  ($Extend |  $Format)*;"
      "$MidNumLetEx    = $MidNumLet    ($Extend |  $Format)*;"
      "$MidLetterEx    = $MidLetter    ($Extend |  $Format)*;"
      "$MidNumEx       = $MidNum       ($Extend |  $Format)*;"
      "$NumericEx      = $Numeric      ($Extend |  $Format)*;"
      "$ExtendNumLetEx = $ExtendNumLet ($Extend |  $Format)*;"

      "$Hiragana       = [\\p{script=Hiragana}];"
      "$Ideographic    = [\\p{Ideographic}];"
      "$HiraganaEx     = $Hiragana     ($Extend |  $Format)*;"
      "$IdeographicEx  = $Ideographic  ($Extend |  $Format)*;"

      "!!forward;"
      "$CR $LF;"
      "[^$CR $LF $Newline]? ($Extend |  $Format)+;"
      "$ALetterEx {200};"
      "$ALetterEx $ALetterEx {200};"
      "%s"  // (Allow|Disallow) Contraction

      "!!reverse;"
      "$BackALetterEx     = ($Format | $Extend)* $ALetterPlus;"
      "$BackMidNumLetEx   = ($Format | $Extend)* $MidNumLet;"
      "$BackNumericEx     = ($Format | $Extend)* $Numeric;"
      "$BackMidNumEx      = ($Format | $Extend)* $MidNum;"
      "$BackMidLetterEx   = ($Format | $Extend)* $MidLetter;"
      "$BackKatakanaEx    = ($Format | $Extend)* $Katakana;"
      "$BackExtendNumLetEx= ($Format | $Extend)* $ExtendNumLet;"
      "$LF $CR;"
      "($Format | $Extend)*  [^$CR $LF $Newline]?;"
      "$BackALetterEx $BackALetterEx;"
      "$BackALetterEx ($BackMidLetterEx | $BackMidNumLetEx) $BackALetterEx;"
      "$BackNumericEx $BackNumericEx;"
      "$BackNumericEx $BackALetterEx;"
      "$BackALetterEx $BackNumericEx;"
      "$BackNumericEx ($BackMidNumEx | $BackMidNumLetEx) $BackNumericEx;"
      "$BackKatakanaEx $BackKatakanaEx;"
      "$BackExtendNumLetEx ($BackALetterEx | $BackNumericEx |"
      " $BackKatakanaEx | $BackExtendNumLetEx);"
      "($BackALetterEx | $BackNumericEx | $BackKatakanaEx)"
      " $BackExtendNumLetEx;"

      "!!safe_reverse;"
      "($Extend | $Format)+ .?;"
      "($MidLetter | $MidNumLet) $BackALetterEx;"
      "($MidNum | $MidNumLet) $BackNumericEx;"

      "!!safe_forward;"
      "($Extend | $Format)+ .?;"
      "($MidLetterEx | $MidNumLetEx) $ALetterEx;"
      "($MidNumEx | $MidNumLetEx) $NumericEx;";

  // Retrieve the script codes used by the given language from ICU. When the
  // given language consists of two or more scripts, we just use the first
  // script. The size of returned script codes is always < 8. Therefore, we use
  // an array of size 8 so we can include all script codes without insufficient
  // buffer errors.
  UErrorCode error = U_ZERO_ERROR;
  UScriptCode script_code[8];
  int scripts = uscript_getCode(language.c_str(), script_code,
                                arraysize(script_code), &error);
  if (U_SUCCESS(error) && scripts >= 1)
    script_code_ = script_code[0];

  // Retrieve the values for $ALetter and $ALetterPlus. We use the dictionary
  // only for the languages which need it (i.e. Korean and Thai) to prevent ICU
  // from returning dictionary words (i.e. Korean or Thai words) for languages
  // which don't need them.
  const char* aletter = uscript_getName(script_code_);
  if (!aletter)
    aletter = "Latin";

  const char kWithDictionary[] =
      "$dictionary   = [:LineBreak = Complex_Context:];"
      "$ALetterPlus  = [$ALetter [$dictionary-$Extend-$Control]];";
  const char kWithoutDictionary[] = "$ALetterPlus  = $ALetter;";
  const char* aletter_plus = kWithoutDictionary;
  if (script_code_ == USCRIPT_HANGUL || script_code_ == USCRIPT_THAI ||
      script_code_ == USCRIPT_LAO || script_code_ == USCRIPT_KHMER)
    aletter_plus = kWithDictionary;

  // Treat numbers as word characters except for Arabic and Hebrew.
  const char* aletter_extra = " [0123456789]";
  if (script_code_ == USCRIPT_HEBREW || script_code_ == USCRIPT_ARABIC)
    aletter_extra = "";

  const char kMidLetterExtra[] = "";
  // For Hebrew, treat single/double quoation marks as MidLetter.
  const char kMidLetterExtraHebrew[] = "\"'";
  const char* midletter_extra = kMidLetterExtra;
  if (script_code_ == USCRIPT_HEBREW)
    midletter_extra = kMidLetterExtraHebrew;

  // Create two custom rule-sets: one allows contraction and the other does not.
  // We save these strings in UTF-16 so we can use it without conversions. (ICU
  // needs UTF-16 strings.)
  const char kAllowContraction[] =
      "$ALetterEx ($MidLetterEx | $MidNumLetEx) $ALetterEx {200};";
  const char kDisallowContraction[] = "";

  ruleset_allow_contraction_ = base::ASCIIToUTF16(
      base::StringPrintf(kRuleTemplate,
                         aletter,
                         aletter_extra,
                         midletter_extra,
                         aletter_plus,
                         kAllowContraction));
  ruleset_disallow_contraction_ = base::ASCIIToUTF16(
      base::StringPrintf(kRuleTemplate,
                         aletter,
                         aletter_extra,
                         midletter_extra,
                         aletter_plus,
                         kDisallowContraction));
}

bool SpellcheckCharAttribute::OutputChar(UChar c,
                                         base::string16* output) const {
  // Call the language-specific function if necessary.
  // Otherwise, we call the default one.
  switch (script_code_) {
    case USCRIPT_ARABIC:
      return OutputArabic(c, output);

    case USCRIPT_HANGUL:
      return OutputHangul(c, output);

    case USCRIPT_HEBREW:
      return OutputHebrew(c, output);

    default:
      return OutputDefault(c, output);
  }
}

bool SpellcheckCharAttribute::OutputArabic(UChar c,
                                           base::string16* output) const {
  // Discard characters not from Arabic alphabets. We also discard vowel marks
  // of Arabic (Damma, Fatha, Kasra, etc.) to prevent our Arabic dictionary from
  // marking an Arabic word including vowel marks as misspelled. (We need to
  // check these vowel marks manually and filter them out since their script
  // codes are USCRIPT_ARABIC.)
  if (0x0621 <= c && c <= 0x064D)
    output->push_back(c);
  return true;
}

bool SpellcheckCharAttribute::OutputHangul(UChar c,
                                           base::string16* output) const {
  // Decompose a Hangul character to a Hangul vowel and consonants used by our
  // spellchecker. A Hangul character of Unicode is a ligature consisting of a
  // Hangul vowel and consonants, e.g. U+AC01 "Gag" consists of U+1100 "G",
  // U+1161 "a", and U+11A8 "g". That is, we can treat each Hangul character as
  // a point of a cubic linear space consisting of (first consonant, vowel, last
  // consonant). Therefore, we can compose a Hangul character from a vowel and
  // two consonants with linear composition:
  //   character =  0xAC00 +
  //                (first consonant - 0x1100) * 28 * 21 +
  //                (vowel           - 0x1161) * 28 +
  //                (last consonant  - 0x11A7);
  // We can also decompose a Hangul character with linear decomposition:
  //   first consonant = (character - 0xAC00) / 28 / 21;
  //   vowel           = (character - 0xAC00) / 28 % 21;
  //   last consonant  = (character - 0xAC00) % 28;
  // This code is copied from Unicode Standard Annex #15
  // <http://unicode.org/reports/tr15> and added some comments.
  const int kSBase = 0xAC00;  // U+AC00: the top of Hangul characters.
  const int kLBase = 0x1100;  // U+1100: the top of Hangul first consonants.
  const int kVBase = 0x1161;  // U+1161: the top of Hangul vowels.
  const int kTBase = 0x11A7;  // U+11A7: the top of Hangul last consonants.
  const int kLCount = 19;     // The number of Hangul first consonants.
  const int kVCount = 21;     // The number of Hangul vowels.
  const int kTCount = 28;     // The number of Hangul last consonants.
  const int kNCount = kVCount * kTCount;
  const int kSCount = kLCount * kNCount;

  int index = c - kSBase;
  if (index < 0 || index >= kSBase + kSCount) {
    // This is not a Hangul syllable. Call the default output function since we
    // should output this character when it is a Hangul syllable.
    return OutputDefault(c, output);
  }

  // This is a Hangul character. Decompose this characters into Hangul vowels
  // and consonants.
  int l = kLBase + index / kNCount;
  int v = kVBase + (index % kNCount) / kTCount;
  int t = kTBase + index % kTCount;
  output->push_back(l);
  output->push_back(v);
  if (t != kTBase)
    output->push_back(t);
  return true;
}

bool SpellcheckCharAttribute::OutputHebrew(UChar c,
                                           base::string16* output) const {
  // Discard characters except Hebrew alphabets. We also discard Hebrew niqquds
  // to prevent our Hebrew dictionary from marking a Hebrew word including
  // niqquds as misspelled. (Same as Arabic vowel marks, we need to check
  // niqquds manually and filter them out since their script codes are
  // USCRIPT_HEBREW.)
  // Pass through ASCII single/double quotation marks and Hebrew Geresh and
  // Gershayim.
  if ((0x05D0 <= c && c <= 0x05EA) || c == 0x22 || c == 0x27 ||
      c == 0x05F4 || c == 0x05F3)
    output->push_back(c);
  return true;
}

bool SpellcheckCharAttribute::OutputDefault(UChar c,
                                            base::string16* output) const {
  // Check the script code of this character and output only if it is the one
  // used by the spellchecker language.
  UErrorCode status = U_ZERO_ERROR;
  UScriptCode script_code = uscript_getScript(c, &status);
  if (script_code == script_code_ || script_code == USCRIPT_COMMON)
    output->push_back(c);
  return true;
}

// SpellcheckWordIterator implementation:

SpellcheckWordIterator::SpellcheckWordIterator()
    : text_(NULL),
      attribute_(NULL),
      iterator_() {
}

SpellcheckWordIterator::~SpellcheckWordIterator() {
  Reset();
}

bool SpellcheckWordIterator::Initialize(
    const SpellcheckCharAttribute* attribute,
    bool allow_contraction) {
  // Create a custom ICU break iterator with empty text used in this object. (We
  // allow setting text later so we can re-use this iterator.)
  DCHECK(attribute);
  const base::string16 rule(attribute->GetRuleSet(allow_contraction));

  // If there is no rule set, the attributes were invalid.
  if (rule.empty())
    return false;

  scoped_ptr<BreakIterator> iterator(new BreakIterator(base::string16(), rule));
  if (!iterator->Init()) {
    // Since we're not passing in any text, the only reason this could fail
    // is if we fail to parse the rules. Since the rules are hardcoded,
    // that would be a bug in this class.
    NOTREACHED() << "failed to open iterator (broken rules)";
    return false;
  }
  iterator_ = std::move(iterator);

  // Set the character attributes so we can normalize the words extracted by
  // this iterator.
  attribute_ = attribute;
  return true;
}

bool SpellcheckWordIterator::IsInitialized() const {
  // Return true iff we have an iterator.
  return !!iterator_;
}

bool SpellcheckWordIterator::SetText(const base::char16* text, size_t length) {
  DCHECK(!!iterator_);

  // Set the text to be split by this iterator.
  if (!iterator_->SetText(text, length)) {
    LOG(ERROR) << "failed to set text";
    return false;
  }

  text_ = text;
  return true;
}

SpellcheckWordIterator::WordIteratorStatus SpellcheckWordIterator::GetNextWord(
    base::string16* word_string,
    int* word_start,
    int* word_length) {
  DCHECK(!!text_);

  word_string->clear();
  *word_start = 0;
  *word_length = 0;

  if (!text_) {
    return IS_END_OF_TEXT;
  }

  // Find a word that can be checked for spelling or a character that can be
  // skipped over. Rather than moving past a skippable character this returns
  // IS_SKIPPABLE and defers handling the character to the calling function.
  while (iterator_->Advance()) {
    const size_t start = iterator_->prev();
    const size_t length = iterator_->pos() - start;
    switch (iterator_->GetWordBreakStatus()) {
      case BreakIterator::IS_WORD_BREAK: {
        if (Normalize(start, length, word_string)) {
          *word_start = start;
          *word_length = length;
          return IS_WORD;
        }
        break;
      }
      case BreakIterator::IS_SKIPPABLE_WORD: {
        *word_string = iterator_->GetString();
        *word_start = start;
        *word_length = length;
        return IS_SKIPPABLE;
      }
      // |iterator_| is RULE_BASED so the break status should never be
      // IS_LINE_OR_CHAR_BREAK.
      case BreakIterator::IS_LINE_OR_CHAR_BREAK: {
        NOTREACHED();
        break;
      }
    }
  }

  // There aren't any more words in the given text.
  return IS_END_OF_TEXT;
}

void SpellcheckWordIterator::Reset() {
  iterator_.reset();
}

bool SpellcheckWordIterator::Normalize(int input_start,
                                       int input_length,
                                       base::string16* output_string) const {
  // We use NFKC (Normalization Form, Compatible decomposition, followed by
  // canonical Composition) defined in Unicode Standard Annex #15 to normalize
  // this token because it it the most suitable normalization algorithm for our
  // spellchecker. Nevertheless, it is not a perfect algorithm for our
  // spellchecker and we need manual normalization as well. The normalized
  // text does not have to be NUL-terminated since its characters are copied to
  // string16, which adds a NUL character when we need.
  icu::UnicodeString input(FALSE, &text_[input_start], input_length);
  UErrorCode status = U_ZERO_ERROR;
  icu::UnicodeString output;
  icu::Normalizer::normalize(input, UNORM_NFKC, 0, output, status);
  if (status != U_ZERO_ERROR && status != U_STRING_NOT_TERMINATED_WARNING)
    return false;

  // Copy the normalized text to the output.
  icu::StringCharacterIterator it(output);
  for (UChar c = it.first(); c != icu::CharacterIterator::DONE; c = it.next())
    attribute_->OutputChar(c, output_string);

  return !output_string->empty();
}
