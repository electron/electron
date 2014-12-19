// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_spell_check_client.h"

#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/converter.h"
#include "third_party/WebKit/public/web/WebTextCheckingCompletion.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

SpellCheckClient::SpellCheckClient(v8::Isolate* isolate,
                                   v8::Handle<v8::Object> provider)
    : isolate_(isolate), provider_(isolate, provider) {}

SpellCheckClient::~SpellCheckClient() {}

void SpellCheckClient::spellCheck(
    const blink::WebString& text,
    int& misspelledOffset,
    int& misspelledLength,
    blink::WebVector<blink::WebString>* optionalSuggestions) {
  std::vector<int> result;
  if (!CallProviderMethod("spellCheck", text, &result))
    return;

  if (result.size() != 2)
    return;

  misspelledOffset = result[0];
  misspelledLength = result[1];
}

void SpellCheckClient::checkTextOfParagraph(
    const blink::WebString& text,
    blink::WebTextCheckingTypeMask mask,
    blink::WebVector<blink::WebTextCheckingResult>* results) {
}

void SpellCheckClient::requestCheckingOfText(
    const blink::WebString& textToCheck,
    const blink::WebVector<uint32_t>& markersInText,
    const blink::WebVector<unsigned>& markerOffsets,
    blink::WebTextCheckingCompletion* completionCallback) {
  if (completionCallback)
    completionCallback->didCancelCheckingText();
}

blink::WebString SpellCheckClient::autoCorrectWord(
    const blink::WebString& misspelledWord) {
  base::string16 result;
  if (!CallProviderMethod("autoCorrectWord", misspelledWord, &result))
    return blink::WebString();

  return result;
}

void SpellCheckClient::showSpellingUI(bool show) {
}

bool SpellCheckClient::isShowingSpellingUI() {
  return false;
}

void SpellCheckClient::updateSpellingUIWithMisspelledWord(
    const blink::WebString& word) {
}

template<class T>
bool SpellCheckClient::CallProviderMethod(const char* method,
                                          const blink::WebString& text,
                                          T* result) {
  v8::HandleScope handle_scope(isolate_);

  v8::Handle<v8::Object> provider = provider_.NewHandle();
  if (!provider->Has(mate::StringToV8(isolate_, method)))
    return false;

  v8::Handle<v8::Value> v8_str =
      mate::ConvertToV8(isolate_, base::string16(text));
  v8::Handle<v8::Value> v8_result =
      node::MakeCallback(isolate_, provider, method, 1, &v8_str);

  return mate::ConvertFromV8(isolate_, v8_result, result);;
}

}  // namespace api

}  // namespace atom
