// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_spell_check_client.h"

namespace atom {

namespace api {

SpellCheckClient::SpellCheckClient(v8::Isolate* isolate,
                                   v8::Local<v8::Object> provider)
    : provider_(isolate, provider) {}

SpellCheckClient::~SpellCheckClient() {}

void SpellCheckClient::spellCheck(
    const blink::WebString& text,
    int& misspelledOffset,
    int& misspelledLength,
    blink::WebVector<blink::WebString>* optionalSuggestions) {
}

void SpellCheckClient::checkTextOfParagraph(
    const blink::WebString&,
    blink::WebTextCheckingTypeMask mask,
    blink::WebVector<blink::WebTextCheckingResult>* results) {
}

void SpellCheckClient::requestCheckingOfText(
    const blink::WebString& textToCheck,
    const blink::WebVector<uint32_t>& markersInText,
    const blink::WebVector<unsigned>& markerOffsets,
    blink::WebTextCheckingCompletion* completionCallback) {
}

blink::WebString SpellCheckClient::autoCorrectWord(
    const blink::WebString& misspelledWord) {
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

}  // namespace api

}  // namespace atom
