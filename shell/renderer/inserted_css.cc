// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/inserted_css.h"

#include "base/strings/strcat.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"

namespace electron {

namespace {

// Blink's keys are decimal numbers, so a key with this prefix can never be
// one of them. Author-origin keys are returned unchanged.
constexpr std::u16string_view kUserOriginKeyPrefix = u"user:";

}  // namespace

std::u16string InsertStyleSheet(blink::WebDocument document,
                                std::string_view css,
                                blink::WebCssOrigin origin) {
  std::u16string key =
      document
          .InsertStyleSheet(blink::WebString::FromUtf8(css), nullptr, origin)
          .Utf16();
  if (origin == blink::WebCssOrigin::kUser)
    return base::StrCat({kUserOriginKeyPrefix, key});
  return key;
}

void RemoveInsertedStyleSheet(blink::WebDocument document,
                              std::u16string_view key) {
  blink::WebCssOrigin origin = blink::WebCssOrigin::kAuthor;
  if (key.starts_with(kUserOriginKeyPrefix)) {
    key.remove_prefix(kUserOriginKeyPrefix.size());
    origin = blink::WebCssOrigin::kUser;
  }
  document.RemoveInsertedStyleSheet(blink::WebString::FromUtf16(key), origin);
}

}  // namespace electron
