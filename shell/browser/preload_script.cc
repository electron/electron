// Copyright (c) 2025 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/preload_script.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/files/file_path.h"
#include "base/strings/pattern.h"
#include "base/uuid.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

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
      // URLPattern requires "scheme://host/path" for standard schemes and
      // rejects it for any other scheme.
      if (result == URLPattern::ParseResult::kWrongSchemeSeparator) {
        const bool has_standard_separator =
            pattern_string.find(url::kStandardSchemeSeparator) !=
            std::string::npos;
        *error += has_standard_separator
                      ? " A custom scheme must be registered as standard to "
                        "be matched with '://'. Use includeGlobs or "
                        "excludeGlobs otherwise."
                      : " A pattern for a standard scheme must use "
                        "'scheme://host/path'.";
      }
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

// The URL to match filters against for a document at |url| with security
// origin |origin| when matching the origin as a fallback. Mirrors
// extensions::ContentScriptInjectionUrlGetter::Get() for
// MatchOriginAsFallbackBehavior::kAlways.
GURL GetOriginFallbackURL(const GURL& url, const url::Origin& origin) {
  static constexpr std::string_view kSchemesWithoutOwnOrigin[] = {
      url::kAboutScheme,
      url::kBlobScheme,
      url::kDataScheme,
      url::kFileSystemScheme,
  };
  if (!std::ranges::contains(kSchemesWithoutOwnOrigin, url.GetScheme()))
    return url;
  // An opaque origin with no precursor has no creator to fall back to, e.g. a
  // browser-initiated navigation to a data: URL.
  const url::SchemeHostPort& tuple = origin.GetTupleOrPrecursorTupleIfOpaque();
  if (!tuple.IsValid())
    return url;
  return tuple.GetURL();
}

}  // namespace

PreloadScript::PreloadScript() = default;
PreloadScript::PreloadScript(const PreloadScript&) = default;
PreloadScript::PreloadScript(PreloadScript&&) = default;
PreloadScript& PreloadScript::operator=(const PreloadScript&) = default;
PreloadScript& PreloadScript::operator=(PreloadScript&&) = default;
PreloadScript::~PreloadScript() = default;

bool PreloadScript::ParseUrlPatterns(std::string* error) {
  // Content scripts require at least one match pattern, and webRequest
  // filters dropped "empty means everything". Keep that unambiguous here.
  if (matches && matches->empty()) {
    *error =
        "Preload script 'matches' must not be empty. Use '<all_urls>' to "
        "match all URLs.";
    return false;
  }
  // Likewise, an empty include list must not mean every URL.
  if (include_globs && include_globs->empty()) {
    *error = "Preload script 'includeGlobs' must not be empty.";
    return false;
  }
  if (!ParsePatterns(matches.value_or(std::vector<std::string>()),
                     &match_patterns_, error) ||
      !ParsePatterns(exclude_matches, &exclude_match_patterns_, error)) {
    return false;
  }
  // An origin has no path, so a pattern with one could never match a document
  // by its origin. Same rule as content scripts.
  if (match_origin_as_fallback) {
    for (const URLPattern& pattern : match_patterns_) {
      if (pattern.path() != "/*") {
        *error = "Invalid URL pattern '" + pattern.GetAsString() +
                 "': the path must be '/*' when matchOriginAsFallback is set";
        return false;
      }
    }
  }
  return true;
}

// Mirrors extensions::UserScript::MatchesURL() for content scripts.
bool PreloadScript::MatchesURL(const GURL& url) const {
  // Fail closed if ParseUrlPatterns() was skipped: unparsed patterns would
  // match every URL and drop every exclusion.
  if (match_patterns_.size() != (matches ? matches->size() : 0u) ||
      exclude_match_patterns_.size() != exclude_matches.size()) {
    return false;
  }
  if (MatchesAnyPattern(exclude_match_patterns_, url))
    return false;
  if (MatchesAnyGlob(exclude_globs, url))
    return false;
  return (!matches || MatchesAnyPattern(match_patterns_, url)) &&
         (!include_globs || MatchesAnyGlob(*include_globs, url));
}

bool PreloadScript::MatchesDocument(const GURL& url,
                                    const url::Origin& origin) const {
  return MatchesURL(match_origin_as_fallback ? GetOriginFallbackURL(url, origin)
                                             : url);
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
  if (script.matches)
    dict.Set("matches", *script.matches);
  if (!script.exclude_matches.empty())
    dict.Set("excludeMatches", script.exclude_matches);
  if (script.include_globs)
    dict.Set("includeGlobs", *script.include_globs);
  if (!script.exclude_globs.empty())
    dict.Set("excludeGlobs", script.exclude_globs);
  if (script.match_origin_as_fallback)
    dict.Set("matchOriginAsFallback", true);
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
  auto get_filter = [&](std::string_view key, std::vector<std::string>* field) {
    v8::Local<v8::Value> value;
    if (!options.Get(key, &value) || value->IsUndefined())
      return true;
    return ConvertFromV8(isolate, value, field);
  };
  // |matches| and |includeGlobs| also record whether they were given, so
  // that an explicit empty list can be told apart from an omitted one.
  auto get_include_filter =
      [&](std::string_view key,
          std::optional<std::vector<std::string>>* field) {
        v8::Local<v8::Value> value;
        if (!options.Get(key, &value) || value->IsUndefined())
          return true;
        return ConvertFromV8(isolate, value, &field->emplace());
      };
  if (!get_include_filter("matches", &out->matches) ||
      !get_filter("excludeMatches", &out->exclude_matches) ||
      !get_include_filter("includeGlobs", &out->include_globs) ||
      !get_filter("excludeGlobs", &out->exclude_globs)) {
    return false;
  }
  options.Get("matchOriginAsFallback", &out->match_origin_as_fallback);
  if (bool deprecated; options.Get("_deprecated", &deprecated)) {
    out->deprecated = deprecated;
  }
  return true;
}

}  // namespace gin
