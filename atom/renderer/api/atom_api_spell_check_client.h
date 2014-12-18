// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_
#define ATOM_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_

#include "native_mate/scoped_persistent.h"
#include "third_party/WebKit/public/web/WebSpellCheckClient.h"

namespace atom {

namespace api {

class SpellCheckClient : public blink::WebSpellCheckClient {
 public:
  SpellCheckClient(v8::Isolate* isolate, v8::Local<v8::Object> provider);
  virtual ~SpellCheckClient();

 private:
  // blink::WebSpellCheckClient:
  void spellCheck(
      const blink::WebString& text,
      int& misspelledOffset,
      int& misspelledLength,
      blink::WebVector<blink::WebString>* optionalSuggestions) override;
  void checkTextOfParagraph(
      const blink::WebString&,
      blink::WebTextCheckingTypeMask mask,
      blink::WebVector<blink::WebTextCheckingResult>* results) override;
  void requestCheckingOfText(
      const blink::WebString& textToCheck,
      const blink::WebVector<uint32_t>& markersInText,
      const blink::WebVector<unsigned>& markerOffsets,
      blink::WebTextCheckingCompletion* completionCallback) override;
  blink::WebString autoCorrectWord(
      const blink::WebString& misspelledWord) override;
  void showSpellingUI(bool show) override;
  bool isShowingSpellingUI() override;
  void updateSpellingUIWithMisspelledWord(
      const blink::WebString& word) override;

  mate::ScopedPersistent<v8::Object> provider_;

  DISALLOW_COPY_AND_ASSIGN(SpellCheckClient);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_SPELL_CHECK_CLIENT_H_
