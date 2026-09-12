// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/header_rules.h"

#include "base/strings/string_util.h"
#include "extensions/common/api/web_request/web_request_resource_type.h"
#include "extensions/common/url_pattern.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "shell/browser/net/url_loader_factory_gate.h"
#include "url/gurl.h"

namespace electron {

namespace {

constexpr int kSchemes = URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS |
                         URLPattern::SCHEME_WS | URLPattern::SCHEME_WSS;

bool ParsePatterns(const base::Value* value,
                   const char* key,
                   bool required,
                   extensions::URLPatternSet* out,
                   std::string* error) {
  if (!value) {
    if (required)
      *error = std::string("'") + key + "' is required";
    return !required;
  }
  const auto* list = value->GetIfList();
  if (!list || (required && list->empty())) {
    *error = std::string("'") + key + "' must be a non-empty array of patterns";
    return false;
  }
  for (const auto& item : *list) {
    const auto* str = item.GetIfString();
    URLPattern pattern(kSchemes);
    if (!str || pattern.Parse(*str) != URLPattern::ParseResult::kSuccess) {
      *error = std::string("Invalid pattern in '") + key + "'";
      return false;
    }
    out->AddPattern(pattern);
  }
  return true;
}

// A rule that sets request headers must name the hosts it applies to.
bool NamesItsHosts(const extensions::URLPatternSet& patterns) {
  for (const auto& pattern : patterns) {
    if (pattern.match_all_urls() || pattern.host().empty() ||
        (pattern.match_subdomains() &&
         pattern.host().find('.') == std::string::npos)) {
      return false;
    }
  }
  return true;
}

}  // namespace

HeaderRules::Rule::Rule() = default;
HeaderRules::Rule::Rule(Rule&&) = default;
HeaderRules::Rule::~Rule() = default;

HeaderRules::HeaderRules() = default;
HeaderRules::~HeaderRules() = default;

// static
scoped_refptr<const HeaderRules> HeaderRules::Compile(
    const base::ListValue& list,
    std::string* error) {
  scoped_refptr<HeaderRules> rules(new HeaderRules());
  for (const auto& item : list) {
    const auto* dict = item.GetIfDict();
    if (!dict) {
      *error = "Each rule must be an object";
      return nullptr;
    }
    Rule rule;
    if (!ParsePatterns(dict->Find("urls"), "urls", true, &rule.urls, error) ||
        !ParsePatterns(dict->Find("excludeUrls"), "excludeUrls", false,
                       &rule.exclude_urls, error)) {
      return nullptr;
    }
    if (const auto* types = dict->FindList("types")) {
      rule.types = 0;
      for (const auto& type : *types) {
        auto parsed = type.is_string()
                          ? ParseResourceTypeName(type.GetString())
                          : extensions::WebRequestResourceType::OTHER;
        if (parsed == extensions::WebRequestResourceType::OTHER) {
          *error = "Invalid value in 'types'";
          return nullptr;
        }
        rule.types |= 1u << static_cast<int>(parsed);
      }
    }
    bool sets_request_header = false;
    if (const auto* headers = dict->FindDict("requestHeaders")) {
      for (const auto [name, value] : *headers) {
        if (!net::HttpUtil::IsValidHeaderName(name) ||
            !(value.is_none() ||
              (value.is_string() &&
               net::HttpUtil::IsValidHeaderValue(value.GetString())))) {
          *error = "Invalid request header '" + name + "'";
          return nullptr;
        }
        if (value.is_string()) {
          sets_request_header = true;
          rule.request.emplace_back(name, value.GetString());
        } else {
          rule.request.emplace_back(name, std::nullopt);
        }
      }
    }
    if (sets_request_header && !NamesItsHosts(rule.urls)) {
      *error =
          "A rule that sets request headers must list the hosts it applies to "
          "in 'urls'";
      return nullptr;
    }
    if (const auto* headers = dict->FindDict("responseHeaders")) {
      for (const auto [name, value] : *headers) {
        if (!net::HttpUtil::IsValidHeaderName(name)) {
          *error = "Invalid response header '" + name + "'";
          return nullptr;
        }
        if (value.is_none()) {
          rule.response.emplace_back(name, std::nullopt);
          continue;
        }
        std::vector<std::string> values;
        if (value.is_string()) {
          values.push_back(value.GetString());
        } else if (value.is_list()) {
          for (const auto& v : value.GetList()) {
            if (!v.is_string()) {
              *error = "Invalid response header '" + name + "'";
              return nullptr;
            }
            values.push_back(v.GetString());
          }
        } else {
          *error = "Invalid response header '" + name + "'";
          return nullptr;
        }
        for (const auto& v : values) {
          if (!net::HttpUtil::IsValidHeaderValue(v)) {
            *error = "Invalid response header '" + name + "'";
            return nullptr;
          }
        }
        rule.response.emplace_back(name, std::move(values));
      }
    }
    rules->has_request_rules_ |= !rule.request.empty();
    rules->has_response_rules_ |= !rule.response.empty();
    rules->rules_.push_back(std::move(rule));
  }
  return rules;
}

bool HeaderRules::Matches(const Rule& rule,
                          const GURL& url,
                          extensions::WebRequestResourceType type) const {
  return (rule.types & (1u << static_cast<int>(type))) &&
         rule.urls.MatchesURL(url) && !rule.exclude_urls.MatchesURL(url);
}

void HeaderRules::ApplyToRequest(const GURL& url,
                                 extensions::WebRequestResourceType type,
                                 net::HttpRequestHeaders* headers,
                                 base::flat_set<std::string>* injected) const {
  base::flat_set<std::string> set_now;
  for (const auto& rule : rules_) {
    if (rule.request.empty() || !Matches(rule, url, type))
      continue;
    for (const auto& [name, value] : rule.request) {
      const std::string key = base::ToLowerASCII(name);
      if (value) {
        headers->SetHeader(name, *value);
        set_now.insert(key);
      } else {
        headers->RemoveHeader(name);
        set_now.erase(key);
      }
    }
  }
  for (const auto& name : *injected) {
    if (!set_now.contains(name))
      headers->RemoveHeader(name);
  }
  *injected = std::move(set_now);
}

scoped_refptr<net::HttpResponseHeaders> HeaderRules::ApplyToResponse(
    const GURL& url,
    extensions::WebRequestResourceType type,
    const net::HttpResponseHeaders& headers) const {
  scoped_refptr<net::HttpResponseHeaders> result;
  for (const auto& rule : rules_) {
    if (rule.response.empty() || !Matches(rule, url, type))
      continue;
    if (!result)
      result =
          base::MakeRefCounted<net::HttpResponseHeaders>(headers.raw_headers());
    for (const auto& [name, values] : rule.response) {
      result->RemoveHeader(name);
      if (values) {
        for (const auto& value : *values)
          result->AddHeader(name, value);
      }
    }
  }
  return result;
}

}  // namespace electron
