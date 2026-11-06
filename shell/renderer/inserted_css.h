// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_INSERTED_CSS_H_
#define ELECTRON_SHELL_RENDERER_INSERTED_CSS_H_

#include <string>
#include <string_view>

#include "third_party/blink/public/web/web_css_origin.h"

namespace blink {
class WebDocument;
}

namespace electron {

// Inserts |css| into |document| with |origin| and returns the key that
// insertCSS() hands back to JS. The key records |origin|, so that
// RemoveInsertedStyleSheet() can remove the sheet with a single call.
std::u16string InsertStyleSheet(blink::WebDocument document,
                                std::string_view css,
                                blink::WebCssOrigin origin);

// Removes a sheet inserted by InsertStyleSheet(), given the key it returned.
void RemoveInsertedStyleSheet(blink::WebDocument document,
                              std::u16string_view key);

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_INSERTED_CSS_H_
