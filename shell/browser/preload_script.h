// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "extensions/common/url_pattern.h"
#include "gin/converter.h"
#include "v8/include/v8-forward.h"

class GURL;

namespace url {
class Origin;
}

namespace electron {

struct PreloadScript {
  enum class ScriptType { kWebFrame, kServiceWorker };

  std::string id;
  ScriptType script_type;
  base::FilePath file_path;

  // URL filters modeled after Chromium content scripts. A preload with no
  // filters runs for every URL. |matches| and |include_globs| are unset when
  // omitted; an explicit empty list is rejected by ParseUrlPatterns().
  std::optional<std::vector<std::string>> matches;
  std::vector<std::string> exclude_matches;
  std::optional<std::vector<std::string>> include_globs;
  std::vector<std::string> exclude_globs;

  // Frame preloads only. Matches a document whose URL carries no origin of
  // its own (about:, blob:, data:, filesystem:) by the origin it inherited
  // from its creator instead, like "match_origin_as_fallback" for content
  // scripts. Without it an about:blank popup escapes the opener's filters.
  bool match_origin_as_fallback = false;

  // If set, use the deprecated validation behavior of Session.setPreloads
  bool deprecated = false;

  PreloadScript();
  PreloadScript(const PreloadScript&);
  PreloadScript(PreloadScript&&);
  PreloadScript& operator=(const PreloadScript&);
  PreloadScript& operator=(PreloadScript&&);
  ~PreloadScript();

  // Parses |matches| and |exclude_matches| into URL patterns. Must succeed
  // before MatchesURL() or MatchesDocument() is used. On failure, |error|
  // describes the first invalid pattern.
  bool ParseUrlPatterns(std::string* error);

  // Whether this preload should run for a document (or service worker scope)
  // at |url|. Uses the same rules as Chromium content scripts: the URL must
  // match |matches| (if any) and |include_globs| (if any), and must not match
  // |exclude_matches| or |exclude_globs|.
  bool MatchesURL(const GURL& url) const;

  // Whether this frame preload should run for a document at |url| with
  // security origin |origin|. Same as MatchesURL(), except that with
  // |match_origin_as_fallback| the filters are matched against the origin (or
  // its precursor if opaque) when |url| has no origin of its own.
  bool MatchesDocument(const GURL& url, const url::Origin& origin) const;

 private:
  std::vector<URLPattern> match_patterns_;
  std::vector<URLPattern> exclude_match_patterns_;
};

}  // namespace electron

namespace gin {

using electron::PreloadScript;

template <>
struct Converter<PreloadScript::ScriptType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const PreloadScript::ScriptType& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PreloadScript::ScriptType* out);
};

template <>
struct Converter<PreloadScript> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const PreloadScript& script);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     PreloadScript* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
