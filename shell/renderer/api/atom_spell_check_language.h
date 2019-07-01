// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_API_ATOM_API_SPELL_CHECK_LANGUAGE_H_
#define SHELL_RENDERER_API_ATOM_API_SPELL_CHECK_LANGUAGE_H_

#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/spellcheck/renderer/spellcheck_worditerator.h"
#include "third_party/blink/public/web/web_text_checking_result.h"

namespace electron {

class SpellcheckLanguage {
 public:
  struct Word {
    blink::WebTextCheckingResult result;
    base::string16 text;
    std::vector<base::string16> contraction_words;
    Word();
    Word(const Word&);
    ~Word();
  };

  explicit SpellcheckLanguage();
  ~SpellcheckLanguage();

  void Init(const std::string& language);

  std::vector<Word> SpellCheckText(const base::string16& text);

  // Initialize |spellcheck_| if that hasn't happened yet.
  bool InitializeIfNeeded();

  // Return true if the underlying spellcheck engine is enabled.
  bool IsEnabled();
  std::string GetLanguageString() { return language_; }

 private:
  // Returns whether or not the given word is a contraction of valid words
  // (e.g. "word:word").
  // Output variable contraction_words will contain individual
  // words in the contraction.
  bool IsContraction(const base::string16& word,
                     std::vector<base::string16>* contraction_words);

  // Represents character attributes used for filtering out characters which
  // are not supported by this SpellCheck object.
  SpellcheckCharAttribute character_attributes_;

  // Represents word iterators used in this spellchecker. The |text_iterator_|
  // splits text provided by WebKit into words, contractions, or concatenated
  // words. The |contraction_iterator_| splits a concatenated word extracted by
  // |text_iterator_| into word components so we can treat a concatenated word
  // consisting only of correct words as a correct word.
  SpellcheckWordIterator text_iterator_;
  SpellcheckWordIterator contraction_iterator_;
  std::string language_;
  DISALLOW_COPY_AND_ASSIGN(SpellcheckLanguage);
};

}  // namespace electron

#endif  // SHELL_RENDERER_API_ATOM_API_SPELL_CHECK_LANGUAGE_H_
