// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/atom_spell_check_language.h"

#include <utility>

#include "base/logging.h"
#include "components/spellcheck/renderer/spellcheck_worditerator.h"
#include "components/spellcheck/renderer/spelling_engine.h"

namespace electron {

SpellcheckLanguage::Word::Word() = default;

SpellcheckLanguage::Word::Word(const Word& word) = default;

SpellcheckLanguage::Word::~Word() = default;

SpellcheckLanguage::SpellcheckLanguage() {}

SpellcheckLanguage::~SpellcheckLanguage() {}

void SpellcheckLanguage::Init(const std::string& language) {
  character_attributes_.SetDefaultLanguage(language);
  text_iterator_.Reset();
  contraction_iterator_.Reset();
  language_ = language;
}

bool SpellcheckLanguage::InitializeIfNeeded() {
  return true;
}

std::vector<SpellcheckLanguage::Word> SpellcheckLanguage::SpellCheckText(
    const base::string16& text) {
  if (!text_iterator_.IsInitialized() &&
      !text_iterator_.Initialize(&character_attributes_, true)) {
    // We failed to initialize text_iterator_, return as spelled correctly.
    VLOG(1) << "Failed to initialize SpellcheckWordIterator";
    return std::vector<Word>();
  }

  if (!contraction_iterator_.IsInitialized() &&
      !contraction_iterator_.Initialize(&character_attributes_, false)) {
    // We failed to initialize the word iterator, return as spelled correctly.
    VLOG(1) << "Failed to initialize contraction_iterator_";
    return std::vector<Word>();
  }

  text_iterator_.SetText(text.c_str(), text.size());

  base::string16 word;
  size_t word_start;
  size_t word_length;
  std::vector<Word> words;
  Word word_entry;
  for (;;) {  // Run until end of text
    const auto status =
        text_iterator_.GetNextWord(&word, &word_start, &word_length);
    if (status == SpellcheckWordIterator::IS_END_OF_TEXT)
      break;
    if (status == SpellcheckWordIterator::IS_SKIPPABLE)
      continue;

    word_entry.result.location = base::checked_cast<int>(word_start);
    word_entry.result.length = base::checked_cast<int>(word_length);
    word_entry.text = word;
    word_entry.contraction_words.clear();
    words.push_back(word_entry);
    // If the given word is a concatenated word of two or more valid words
    // (e.g. "hello:hello"), we should treat it as a valid word.
    if (IsContraction(word_entry.text, &word_entry.contraction_words)) {
      for (const auto& w : word_entry.contraction_words) {
        Word cw;
        cw.text = w;
        cw.result = word_entry.result;
        words.push_back(cw);
      }
    }
  }
  return words;
}

// Returns whether or not the given string is a contraction.
// This function is a fall-back when the SpellcheckWordIterator class
// returns a concatenated word which is not in the selected dictionary
// (e.g. "in'n'out") but each word is valid.
// Output variable contraction_words will contain individual
// words in the contraction.
bool SpellcheckLanguage::IsContraction(
    const base::string16& contraction,
    std::vector<base::string16>* contraction_words) {
  DCHECK(contraction_iterator_.IsInitialized());

  contraction_iterator_.SetText(contraction.c_str(), contraction.length());

  base::string16 word;
  size_t word_start;
  size_t word_length;
  for (auto status =
           contraction_iterator_.GetNextWord(&word, &word_start, &word_length);
       status != SpellcheckWordIterator::IS_END_OF_TEXT;
       status = contraction_iterator_.GetNextWord(&word, &word_start,
                                                  &word_length)) {
    if (status == SpellcheckWordIterator::IS_SKIPPABLE)
      continue;

    contraction_words->push_back(word);
  }
  return contraction_words->size() > 1;
}

bool SpellcheckLanguage::IsEnabled() {
  return true;
}

}  // namespace electron