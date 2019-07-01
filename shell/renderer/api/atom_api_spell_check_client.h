// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_
#define SHELL_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/spellcheck/renderer/spellcheck_worditerator.h"
#include "native_mate/scoped_persistent.h"
#include "shell/renderer/api/atom_spell_check_language.h"
#include "third_party/blink/public/platform/web_spell_check_panel_host_client.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_text_check_client.h"
#include "third_party/blink/public/web/web_text_checking_completion.h"
#include "third_party/blink/public/web/web_text_checking_result.h"

namespace blink {
// struct WebTextCheckingResult;
// class WebTextCheckingCompletion;
}  // namespace blink

namespace electron {

namespace api {

class SpellCheckClient : public blink::WebSpellCheckPanelHostClient,
                         public blink::WebTextCheckClient,
                         public base::SupportsWeakPtr<SpellCheckClient> {
 public:
  SpellCheckClient(const std::vector<std::string>& languages,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> provider);
  ~SpellCheckClient() override;

 private:
  class SpellcheckRequest;
  // blink::WebTextCheckClient:
  void RequestCheckingOfText(const blink::WebString& textToCheck,
                             std::unique_ptr<blink::WebTextCheckingCompletion>
                                 completionCallback) override;
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

  void AddSpellcheckLanguage(const std::string& language);

  // Run through the word iterator and send out requests
  // to the JS API for checking spellings of words in the current
  // request.
  void SpellCheckText();

  // Call JavaScript to check spelling a word.
  // The javascript function will callback OnSpellCheckDone
  // with the results of all the misspelled words.
  void SpellCheckWords(
      const SpellCheckScope& scope,
      const std::map<std::string, std::vector<SpellcheckLanguage::Word>>&
          lang_words);

  // Callback for the JS API which returns the list of misspelled words.
  void OnSpellCheckDone(
      const std::map<std::string, std::vector<SpellcheckLanguage::Word>>&
          lang_words,
      const std::vector<base::string16>& misspelled_words);

  // The parameters of a pending background-spellchecking request.
  // (When WebKit sends two or more requests, we cancel the previous
  // requests so we do not have to use vectors.)
  std::unique_ptr<SpellcheckRequest> pending_request_param_;

  // A vector of objects used to actually check spelling, one for each enabled
  // language.
  std::vector<std::unique_ptr<SpellcheckLanguage>> languages_;

  v8::Isolate* isolate_;
  v8::Persistent<v8::Context> context_;
  mate::ScopedPersistent<v8::Object> provider_;
  mate::ScopedPersistent<v8::Function> spell_check_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckClient);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_
