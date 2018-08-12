// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_
#define ATOM_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "chrome/renderer/spellchecker/spellcheck_worditerator.h"
#include "native_mate/scoped_persistent.h"
#include "third_party/blink/public/platform/web_spell_check_panel_host_client.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_text_check_client.h"

namespace blink {
struct WebTextCheckingResult;
class WebTextCheckingCompletion;
}  // namespace blink

namespace atom {

namespace api {

class SpellCheckClient : public blink::WebSpellCheckPanelHostClient,
                         public blink::WebTextCheckClient,
                         public base::SupportsWeakPtr<SpellCheckClient> {
 public:
  SpellCheckClient(const std::string& language,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> provider);
  ~SpellCheckClient() override;

 private:
  class SpellcheckRequest;
  // blink::WebTextCheckClient:
  void RequestCheckingOfText(
      const blink::WebString& textToCheck,
      blink::WebTextCheckingCompletion* completionCallback) override;
  bool IsSpellCheckingEnabled() const override;

  // blink::WebSpellCheckPanelHostClient:
  void ShowSpellingUI(bool show) override;
  bool IsShowingSpellingUI() override;
  void UpdateSpellingUIWithMisspelledWord(
      const blink::WebString& word) override;

  struct SpellCheckScope {
    v8::HandleScope handle_scope_;
    v8::Context::Scope context_scope_;
    v8::Local<v8::Object> provider_;
    v8::Local<v8::Function> spell_check_;

    explicit SpellCheckScope(const SpellCheckClient& client);
    ~SpellCheckScope();
  };

  // Run through the word iterator and send out requests
  // to the JS API for checking spellings of words in the current
  // request.
  void SpellCheckText();

  // Call JavaScript to check spelling a word.
  // The javascript function will callback OnSpellCheckDone
  // with the results of all the misspelt words.
  void SpellCheckWords(const SpellCheckScope& scope,
                       const std::vector<base::string16>& words);

  // Returns whether or not the given word is a contraction of valid words
  // (e.g. "word:word").
  // Output variable contraction_words will contain individual
  // words in the contraction.
  bool IsContraction(const SpellCheckScope& scope,
                     const base::string16& word,
                     std::vector<base::string16>* contraction_words);

  // Callback for the JS API which returns the list of misspelt words.
  void OnSpellCheckDone(const std::vector<base::string16>& misspelt_words);

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

  // The parameters of a pending background-spellchecking request.
  // (When WebKit sends two or more requests, we cancel the previous
  // requests so we do not have to use vectors.)
  std::unique_ptr<SpellcheckRequest> pending_request_param_;

  v8::Isolate* isolate_;
  v8::Persistent<v8::Context> context_;
  mate::ScopedPersistent<v8::Object> provider_;
  mate::ScopedPersistent<v8::Function> spell_check_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckClient);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_
