// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/api/atom_api_spell_check_client.h"

#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/logging.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "third_party/WebKit/public/web/WebTextCheckingCompletion.h"
#include "third_party/WebKit/public/web/WebTextCheckingResult.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<blink::WebTextCheckingResult> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     blink::WebTextCheckingResult* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    return dict.Get("location", &(out->location)) &&
           dict.Get("length", &(out->length));
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {

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

SpellCheckClient::SpellCheckClient(v8::Isolate* isolate,
                                   v8::Handle<v8::Object> provider)
    : isolate_(isolate), provider_(isolate, provider) {}

SpellCheckClient::~SpellCheckClient() {}

void SpellCheckClient::spellCheck(
    const blink::WebString& text,
    int& misspelledOffset,
    int& misspelledLength,
    blink::WebVector<blink::WebString>* optionalSuggestions) {
  blink::WebTextCheckingResult result;
  if (!CallProviderMethod("spellCheck", text, &result))
    return;

  misspelledOffset = result.location;
  misspelledLength = result.length;
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
  v8::HandleScope handle_scope(isolate_);
  v8::Handle<v8::Object> provider = provider_.NewHandle();
  if (!provider->Has(mate::StringToV8(isolate_, "requestCheckingOfText"))) {
    completionCallback->didCancelCheckingText();
    return;
  }

  base::string16 text(textToCheck);
  if (text.empty() || !HasWordCharacters(text, 0)) {
    completionCallback->didCancelCheckingText();
    return;
  }

  std::vector<blink::WebTextCheckingResult> result;
  if (!CallProviderMethod("requestCheckingOfText", textToCheck, &result)) {
    completionCallback->didCancelCheckingText();
    return;
  }

  completionCallback->didFinishCheckingText(result);
  return;
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
