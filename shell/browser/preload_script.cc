// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/preload_script.h"

#include <utility>

#include "base/check_op.h"
#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/strings/pattern.h"
#include "base/uuid.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "url/gurl.h"

namespace electron {

namespace {

bool ParsePatterns(const std::vector<std::string>& patterns,
                   std::vector<URLPattern>* out,
                   std::string* error) {
  out->clear();
  for (const std::string& pattern_string : patterns) {
    // Electron apps commonly serve content from custom protocols, so accept
    // any scheme rather than the extension-restricted default set.
    URLPattern pattern(URLPattern::SCHEME_ALL);
    const URLPattern::ParseResult result = pattern.Parse(pattern_string);
    if (result != URLPattern::ParseResult::kSuccess) {
      *error = "Invalid URL pattern '" + pattern_string +
               "': " + URLPattern::GetParseResultString(result);
      return false;
    }
    out->push_back(std::move(pattern));
  }
  return true;
}

bool MatchesAnyPattern(const std::vector<URLPattern>& patterns,
                       const GURL& url) {
  for (const auto& pattern : patterns) {
    if (pattern.MatchesURL(url))
      return true;
  }
  return false;
}

bool MatchesAnyGlob(const std::vector<std::string>& globs, const GURL& url) {
  for (const auto& glob : globs) {
    if (base::MatchPattern(url.spec(), glob))
      return true;
  }
  return false;
}

}  // namespace

PreloadScript::PreloadScript() = default;
PreloadScript::PreloadScript(const PreloadScript&) = default;
PreloadScript::PreloadScript(PreloadScript&&) = default;
PreloadScript& PreloadScript::operator=(const PreloadScript&) = default;
PreloadScript& PreloadScript::operator=(PreloadScript&&) = default;
PreloadScript::~PreloadScript() = default;

bool PreloadScript::ParseUrlPatterns(std::string* error) {
  return ParsePatterns(matches, &match_patterns_, error) &&
         ParsePatterns(exclude_matches, &exclude_match_patterns_, error);
}

// Mirrors extensions::UserScript::MatchesURL() for content scripts.
bool PreloadScript::MatchesURL(const GURL& url) const {
  // Unparsed patterns would silently match every URL.
  DCHECK_EQ(match_patterns_.size(), matches.size());
  DCHECK_EQ(exclude_match_patterns_.size(), exclude_matches.size());
  if (MatchesAnyPattern(exclude_match_patterns_, url))
    return false;
  if (MatchesAnyGlob(exclude_globs, url))
    return false;
  return (match_patterns_.empty() || MatchesAnyPattern(match_patterns_, url)) &&
         (include_globs.empty() || MatchesAnyGlob(include_globs, url));
}

}  // namespace electron

namespace gin {

using electron::PreloadScript;

// static
v8::Local<v8::Value> Converter<PreloadScript::ScriptType>::ToV8(
    v8::Isolate* isolate,
    const PreloadScript::ScriptType& in) {
  using Val = PreloadScript::ScriptType;
  static constexpr auto Lookup = base::MakeFixedFlatMap<Val, std::string_view>({
      {Val::kWebFrame, "frame"},
      {Val::kServiceWorker, "service-worker"},
  });
  return StringToV8(isolate, Lookup.at(in));
}

// static
bool Converter<PreloadScript::ScriptType>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    PreloadScript::ScriptType* out) {
  using Val = PreloadScript::ScriptType;
  static constexpr auto Lookup = base::MakeFixedFlatMap<std::string_view, Val>({
      {"frame", Val::kWebFrame},
      {"service-worker", Val::kServiceWorker},
  });
  return FromV8WithLookup(isolate, val, Lookup, out);
}

// static
v8::Local<v8::Value> Converter<PreloadScript>::ToV8(
    v8::Isolate* isolate,
    const PreloadScript& script) {
  gin::Dictionary dict(isolate, v8::Object::New(isolate));
  dict.Set("filePath", script.file_path.AsUTF8Unsafe());
  dict.Set("id", script.id);
  dict.Set("type", script.script_type);
  if (!script.matches.empty())
    dict.Set("matches", script.matches);
  if (!script.exclude_matches.empty())
    dict.Set("excludeMatches", script.exclude_matches);
  if (!script.include_globs.empty())
    dict.Set("includeGlobs", script.include_globs);
  if (!script.exclude_globs.empty())
    dict.Set("excludeGlobs", script.exclude_globs);
  return ConvertToV8(isolate, dict).As<v8::Object>();
}

// static
bool Converter<PreloadScript>::FromV8(v8::Isolate* isolate,
                                      v8::Local<v8::Value> val,
                                      PreloadScript* out) {
  gin_helper::Dictionary options;
  if (!ConvertFromV8(isolate, val, &options))
    return false;
  if (PreloadScript::ScriptType script_type;
      options.Get("type", &script_type)) {
    out->script_type = script_type;
  } else {
    return false;
  }
  if (base::FilePath file_path; options.Get("filePath", &file_path)) {
    out->file_path = file_path;
  } else {
    return false;
  }
  if (std::string id; options.Get("id", &id)) {
    out->id = id;
  } else {
    out->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  }
  // URL filters are optional, but reject a present value that isn't a
  // string array rather than silently running the preload everywhere.
  for (auto [key, field] : {std::pair{"matches", &out->matches},
                            std::pair{"excludeMatches", &out->exclude_matches},
                            std::pair{"includeGlobs", &out->include_globs},
                            std::pair{"excludeGlobs", &out->exclude_globs}}) {
    v8::Local<v8::Value> value;
    if (options.Get(key, &value) && !value->IsUndefined() &&
        !ConvertFromV8(isolate, value, field)) {
      return false;
    }
  }
  if (bool deprecated; options.Get("_deprecated", &deprecated)) {
    out->deprecated = deprecated;
  }
  return true;
}

}  // namespace gin
