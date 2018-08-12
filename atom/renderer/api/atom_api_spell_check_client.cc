// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_spell_check_client.h"

#include <algorithm>
#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/renderer/spellchecker/spellcheck_worditerator.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "native_mate/function_template.h"
#include "third_party/blink/public/web/web_text_checking_completion.h"
#include "third_party/blink/public/web/web_text_checking_result.h"
#include "third_party/icu/source/common/unicode/uscript.h"

namespace atom {

namespace api {

namespace {

bool HasWordCharacters(const base::string16& text, int index) {
  const base::char16* data = text.data();
  int length = text.length();
  while (index < length) {
    uint32_t code = 0;
    U16_NEXT(data, index, length, code);
    UErrorCode error = U_ZERO_ERROR;
    if (uscript_getScript(code, &error) != USCRIPT_COMMON)
      return true;
  }
  return false;
}

}  // namespace

class SpellCheckClient::SpellcheckRequest {
 public:
  // Map of individual words to list of occurrences in text
  using WordMap =
      std::map<base::string16, std::vector<blink::WebTextCheckingResult>>;

  SpellcheckRequest(const base::string16& text,
                    blink::WebTextCheckingCompletion* completion)
      : text_(text), completion_(completion) {
    DCHECK(completion);
  }
  ~SpellcheckRequest() {}

  base::string16& text() { return text_; }
  blink::WebTextCheckingCompletion* completion() { return completion_; }
  WordMap& wordmap() { return word_map_; }

 private:
  base::string16 text_;  // Text to be checked in this task.
  WordMap word_map_;     // WordMap to hold distinct words in text
  // The interface to send the misspelled ranges to WebKit.
  blink::WebTextCheckingCompletion* completion_;

  DISALLOW_COPY_AND_ASSIGN(SpellcheckRequest);
};

SpellCheckClient::SpellCheckClient(const std::string& language,
                                   v8::Isolate* isolate,
                                   v8::Local<v8::Object> provider)
    : pending_request_param_(nullptr),
      isolate_(isolate),
      context_(isolate, isolate->GetCurrentContext()),
      provider_(isolate, provider) {
  DCHECK(!context_.IsEmpty());

  character_attributes_.SetDefaultLanguage(language);

  // Persistent the method.
  mate::Dictionary dict(isolate, provider);
  dict.Get("spellCheck", &spell_check_);
}

SpellCheckClient::~SpellCheckClient() {
  context_.Reset();
}

void SpellCheckClient::RequestCheckingOfText(
    const blink::WebString& textToCheck,
    blink::WebTextCheckingCompletion* completionCallback) {
  base::string16 text(textToCheck.Utf16());
  // Ignore invalid requests.
  if (text.empty() || !HasWordCharacters(text, 0)) {
    completionCallback->DidCancelCheckingText();
    return;
  }

  // Clean up the previous request before starting a new request.
  if (pending_request_param_) {
    pending_request_param_->completion()->DidCancelCheckingText();
  }

  pending_request_param_.reset(new SpellcheckRequest(text, completionCallback));

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SpellCheckClient::SpellCheckText, AsWeakPtr()));
}

bool SpellCheckClient::IsSpellCheckingEnabled() const {
  return true;
}

void SpellCheckClient::ShowSpellingUI(bool show) {}

bool SpellCheckClient::IsShowingSpellingUI() {
  return false;
}

void SpellCheckClient::UpdateSpellingUIWithMisspelledWord(
    const blink::WebString& word) {}

void SpellCheckClient::SpellCheckText() {
  const auto& text = pending_request_param_->text();
  if (text.empty() || spell_check_.IsEmpty()) {
    pending_request_param_->completion()->DidCancelCheckingText();
    pending_request_param_ = nullptr;
    return;
  }

  if (!text_iterator_.IsInitialized() &&
      !text_iterator_.Initialize(&character_attributes_, true)) {
    // We failed to initialize text_iterator_, return as spelled correctly.
    VLOG(1) << "Failed to initialize SpellcheckWordIterator";
    return;
  }

  if (!contraction_iterator_.IsInitialized() &&
      !contraction_iterator_.Initialize(&character_attributes_, false)) {
    // We failed to initialize the word iterator, return as spelled correctly.
    VLOG(1) << "Failed to initialize contraction_iterator_";
    return;
  }

  text_iterator_.SetText(text.c_str(), text.size());

  SpellCheckScope scope(*this);
  base::string16 word;
  int word_start;
  int word_length;
  std::vector<base::string16> words;
  auto& word_map = pending_request_param_->wordmap();
  for (auto status =
           text_iterator_.GetNextWord(&word, &word_start, &word_length);
       status != SpellcheckWordIterator::IS_END_OF_TEXT;
       status = text_iterator_.GetNextWord(&word, &word_start, &word_length)) {
    if (status == SpellcheckWordIterator::IS_SKIPPABLE)
      continue;

    // If the given word is a concatenated word of two or more valid words
    // (e.g. "hello:hello"), we should treat it as a valid word.
    std::vector<base::string16> contraction_words;
    if (!IsContraction(scope, word, &contraction_words)) {
      blink::WebTextCheckingResult result;
      result.location = word_start;
      result.length = word_length;
      words.push_back(word);
      word_map[word].push_back(result);
    } else {
      // For a contraction, we want check the spellings of each individual
      // part, but mark the entire word incorrect if any part is misspelt
      // Hence, we use the same word_start and word_length values for every
      // part of the contraction.
      for (const auto& w : contraction_words) {
        blink::WebTextCheckingResult result;
        result.location = word_start;
        result.length = word_length;
        words.push_back(w);
        word_map[w].push_back(result);
      }
    }
  }

  // Send out all the words data to the spellchecker to check
  SpellCheckWords(scope, words);
}

void SpellCheckClient::OnSpellCheckDone(
    const std::vector<base::string16>& misspelt_words) {
  std::vector<blink::WebTextCheckingResult> results;
  const auto& completion_handler = pending_request_param_->completion();

  auto& word_map = pending_request_param_->wordmap();

  for (const auto& word : misspelt_words) {
    auto iter = word_map.find(word);
    if (iter != word_map.end()) {
      auto& words = iter->second;
      results.insert(results.end(), words.begin(), words.end());
      words.clear();
    }
  }
  completion_handler->DidFinishCheckingText(results);
  pending_request_param_ = nullptr;
}

void SpellCheckClient::SpellCheckWords(
    const SpellCheckScope& scope,
    const std::vector<base::string16>& words) {
  DCHECK(!scope.spell_check_.IsEmpty());

  v8::Local<v8::FunctionTemplate> templ = mate::CreateFunctionTemplate(
      isolate_, base::Bind(&SpellCheckClient::OnSpellCheckDone, AsWeakPtr()));

  v8::Local<v8::Value> args[] = {mate::ConvertToV8(isolate_, words),
                                 templ->GetFunction()};
  // Call javascript with the words and the callback function
  scope.spell_check_->Call(scope.provider_, 2, args);
}

// Returns whether or not the given string is a contraction.
// This function is a fall-back when the SpellcheckWordIterator class
// returns a concatenated word which is not in the selected dictionary
// (e.g. "in'n'out") but each word is valid.
// Output variable contraction_words will contain individual
// words in the contraction.
bool SpellCheckClient::IsContraction(
    const SpellCheckScope& scope,
    const base::string16& contraction,
    std::vector<base::string16>* contraction_words) {
  DCHECK(contraction_iterator_.IsInitialized());

  contraction_iterator_.SetText(contraction.c_str(), contraction.length());

  base::string16 word;
  int word_start;
  int word_length;
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

SpellCheckClient::SpellCheckScope::SpellCheckScope(
    const SpellCheckClient& client)
    : handle_scope_(client.isolate_),
      context_scope_(
          v8::Local<v8::Context>::New(client.isolate_, client.context_)),
      provider_(client.provider_.NewHandle()),
      spell_check_(client.spell_check_.NewHandle()) {}

SpellCheckClient::SpellCheckScope::~SpellCheckScope() = default;

}  // namespace api

}  // namespace atom
