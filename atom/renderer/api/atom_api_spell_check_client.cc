// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_spell_check_client.h"

#include <algorithm>
#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "third_party/WebKit/public/web/WebTextCheckingCompletion.h"
#include "third_party/WebKit/public/web/WebTextCheckingResult.h"

namespace atom {

namespace api {

namespace {

const int kMaxAutoCorrectWordSize = 8;

bool HasWordCharacters(const base::string16& text, int index) {
  const base::char16* data = text.data();
  int length = text.length();
  while (index < length) {
    uint32 code = 0;
    U16_NEXT(data, index, length, code);
    UErrorCode error = U_ZERO_ERROR;
    if (uscript_getScript(code, &error) != USCRIPT_COMMON)
      return true;
  }
  return false;
}

}  // namespace

SpellCheckClient::SpellCheckClient(const std::string& language,
                                   bool auto_spell_correct_turned_on,
                                   v8::Isolate* isolate,
                                   v8::Local<v8::Object> provider)
    : auto_spell_correct_turned_on_(auto_spell_correct_turned_on),
      isolate_(isolate),
      provider_(isolate, provider) {
  character_attributes_.SetDefaultLanguage(language);

  // Persistent the method.
  mate::Dictionary dict(isolate, provider);
  dict.Get("spellCheck", &spell_check_);
}

SpellCheckClient::~SpellCheckClient() {}

void SpellCheckClient::spellCheck(
    const blink::WebString& text,
    int& misspelling_start,
    int& misspelling_len,
    blink::WebVector<blink::WebString>* optional_suggestions) {
  std::vector<blink::WebTextCheckingResult> results;
  SpellCheckText(base::string16(text), true, &results);
  if (results.size() == 1) {
    misspelling_start = results[0].location;
    misspelling_len = results[0].length;
  }
}

void SpellCheckClient::checkTextOfParagraph(
    const blink::WebString& text,
    blink::WebTextCheckingTypeMask mask,
    blink::WebVector<blink::WebTextCheckingResult>* results) {
  if (!results)
    return;

  if (!(mask & blink::WebTextCheckingTypeSpelling))
    return;

  NOTREACHED() << "checkTextOfParagraph should never be called";
}

void SpellCheckClient::requestCheckingOfText(
    const blink::WebString& textToCheck,
    const blink::WebVector<uint32_t>& markersInText,
    const blink::WebVector<unsigned>& markerOffsets,
    blink::WebTextCheckingCompletion* completionCallback) {
  base::string16 text(textToCheck);
  if (text.empty() || !HasWordCharacters(text, 0)) {
    completionCallback->didCancelCheckingText();
    return;
  }

  std::vector<blink::WebTextCheckingResult> results;
  SpellCheckText(text, false, &results);
  completionCallback->didFinishCheckingText(results);
}

blink::WebString SpellCheckClient::autoCorrectWord(
    const blink::WebString& misspelledWord) {
  if (auto_spell_correct_turned_on_)
    return GetAutoCorrectionWord(base::string16(misspelledWord));
  else
    return blink::WebString();
}

void SpellCheckClient::showSpellingUI(bool show) {
}

bool SpellCheckClient::isShowingSpellingUI() {
  return false;
}

void SpellCheckClient::updateSpellingUIWithMisspelledWord(
    const blink::WebString& word) {
}

void SpellCheckClient::SpellCheckText(
    const base::string16& text,
    bool stop_at_first_result,
    std::vector<blink::WebTextCheckingResult>* results) {
  if (text.length() == 0 || spell_check_.IsEmpty())
    return;

  base::string16 word;
  int word_start;
  int word_length;
  if (!text_iterator_.IsInitialized() &&
      !text_iterator_.Initialize(&character_attributes_, true)) {
      // We failed to initialize text_iterator_, return as spelled correctly.
      VLOG(1) << "Failed to initialize SpellcheckWordIterator";
      return;
  }

  base::string16 in_word(text);
  text_iterator_.SetText(in_word.c_str(), in_word.size());
  while (text_iterator_.GetNextWord(&word, &word_start, &word_length)) {
    // Found a word (or a contraction) that the spellchecker can check the
    // spelling of.
    if (SpellCheckWord(word))
      continue;

    // If the given word is a concatenated word of two or more valid words
    // (e.g. "hello:hello"), we should treat it as a valid word.
    if (IsValidContraction(word))
      continue;

    blink::WebTextCheckingResult result;
    result.location = word_start;
    result.length = word_length;
    results->push_back(result);

    if (stop_at_first_result)
      return;
  }
}

bool SpellCheckClient::SpellCheckWord(const base::string16& word_to_check) {
  if (spell_check_.IsEmpty())
    return true;

  v8::HandleScope handle_scope(isolate_);
  v8::Local<v8::Value> word = mate::ConvertToV8(isolate_, word_to_check);
  v8::Local<v8::Value> result = spell_check_.NewHandle()->Call(
      provider_.NewHandle(), 1, &word);

  if (result->IsBoolean())
    return result->BooleanValue();
  else
    return true;
}

base::string16 SpellCheckClient::GetAutoCorrectionWord(
    const base::string16& word) {
  base::string16 autocorrect_word;

  int word_length = static_cast<int>(word.size());
  if (word_length < 2 || word_length > kMaxAutoCorrectWordSize)
    return autocorrect_word;

  base::char16 misspelled_word[kMaxAutoCorrectWordSize + 1];
  const base::char16* word_char = word.c_str();
  for (int i = 0; i <= kMaxAutoCorrectWordSize; ++i) {
    if (i >= word_length)
      misspelled_word[i] = 0;
    else
      misspelled_word[i] = word_char[i];
  }

  // Swap adjacent characters and spellcheck.
  int misspelling_start, misspelling_len;
  for (int i = 0; i < word_length - 1; i++) {
    // Swap.
    std::swap(misspelled_word[i], misspelled_word[i + 1]);

    // Check spelling.
    misspelling_start = misspelling_len = 0;
    spellCheck(blink::WebString(misspelled_word, word_length),
               misspelling_start,
               misspelling_len,
               NULL);

    // Make decision: if only one swap produced a valid word, then we want to
    // return it. If we found two or more, we don't do autocorrection.
    if (misspelling_len == 0) {
      if (autocorrect_word.empty()) {
        autocorrect_word.assign(misspelled_word);
      } else {
        autocorrect_word.clear();
        break;
      }
    }

    // Restore the swapped characters.
    std::swap(misspelled_word[i], misspelled_word[i + 1]);
  }
  return autocorrect_word;
}

// Returns whether or not the given string is a valid contraction.
// This function is a fall-back when the SpellcheckWordIterator class
// returns a concatenated word which is not in the selected dictionary
// (e.g. "in'n'out") but each word is valid.
bool SpellCheckClient::IsValidContraction(const base::string16& contraction) {
  if (!contraction_iterator_.IsInitialized() &&
      !contraction_iterator_.Initialize(&character_attributes_, false)) {
    // We failed to initialize the word iterator, return as spelled correctly.
    VLOG(1) << "Failed to initialize contraction_iterator_";
    return true;
  }

  contraction_iterator_.SetText(contraction.c_str(), contraction.length());

  base::string16 word;
  int word_start;
  int word_length;

  while (contraction_iterator_.GetNextWord(&word, &word_start, &word_length)) {
    if (!SpellCheckWord(word))
      return false;
  }
  return true;
}

}  // namespace api

}  // namespace atom
