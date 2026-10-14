// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_SCRIPT_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "extensions/common/url_pattern.h"
#include "gin/converter.h"
#include "v8/include/v8-forward.h"

class GURL;

namespace electron {

struct PreloadScript {
  enum class ScriptType { kWebFrame, kServiceWorker };

  std::string id;
  ScriptType script_type;
  base::FilePath file_path;

  // URL filters modeled after Chromium content scripts. A preload with no
  // filters runs for every URL.
  std::vector<std::string> matches;
  std::vector<std::string> exclude_matches;
  std::vector<std::string> include_globs;
  std::vector<std::string> exclude_globs;

  // If set, use the deprecated validation behavior of Session.setPreloads
  bool deprecated = false;

  PreloadScript();
  PreloadScript(const PreloadScript&);
  PreloadScript(PreloadScript&&);
  PreloadScript& operator=(const PreloadScript&);
  PreloadScript& operator=(PreloadScript&&);
  ~PreloadScript();

  // Parses |matches| and |exclude_matches| into URL patterns. Must succeed
  // before MatchesURL() is used. On failure, |error| describes the first
  // invalid pattern.
  bool ParseUrlPatterns(std::string* error);

  // Whether this preload should run for a document (or service worker scope)
  // at |url|. Uses the same rules as Chromium content scripts: the URL must
  // match |matches| (if any) and |include_globs| (if any), and must not match
  // |exclude_matches| or |exclude_globs|.
  bool MatchesURL(const GURL& url) const;

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
