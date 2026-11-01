// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_HEADER_RULES_H_
#define ELECTRON_SHELL_BROWSER_NET_HEADER_RULES_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "extensions/common/url_pattern_set.h"

class GURL;

namespace extensions {
enum class WebRequestResourceType : uint8_t;
}

namespace net {
class HttpRequestHeaders;
class HttpResponseHeaders;
}  // namespace net

namespace electron {

// A compiled, immutable set of session.webRequest.setHeaderRules() rules.
// Built on the UI thread, read on the UI and IO threads.
class HeaderRules : public base::RefCountedThreadSafe<HeaderRules> {
 public:
  struct Rule {
    Rule();
    Rule(Rule&&);
    ~Rule();
    extensions::URLPatternSet urls;
    extensions::URLPatternSet exclude_urls;
    uint32_t types = ~0u;
    // nullopt value = remove the header.
    std::vector<std::pair<std::string, std::optional<std::string>>> request;
    std::vector<std::pair<std::string, std::optional<std::vector<std::string>>>>
        response;
  };

  // Parses the value passed to setHeaderRules(); on failure returns null and
  // sets `error`.
  static scoped_refptr<const HeaderRules> Compile(const base::ListValue& rules,
                                                  std::string* error);

  bool empty() const { return rules_.empty(); }
  bool has_request_rules() const { return has_request_rules_; }
  bool has_response_rules() const { return has_response_rules_; }

  // Sets and removes request headers for one leg of a request to `url`.
  // `injected` carries, across legs, the header names a rule set earlier on
  // this request; any of them that no rule sets on this leg is removed, so a
  // credential never rides a redirect to somewhere the rules do not cover.
  void ApplyToRequest(const GURL& url,
                      extensions::WebRequestResourceType type,
                      net::HttpRequestHeaders* headers,
                      base::flat_set<std::string>* injected) const;

  // Returns the rewritten response headers for `url`, or null when no rule
  // touches them.
  scoped_refptr<net::HttpResponseHeaders> ApplyToResponse(
      const GURL& url,
      extensions::WebRequestResourceType type,
      const net::HttpResponseHeaders& headers) const;

 private:
  friend class base::RefCountedThreadSafe<HeaderRules>;
  HeaderRules();
  ~HeaderRules();

  bool Matches(const Rule& rule,
               const GURL& url,
               extensions::WebRequestResourceType type) const;

  std::vector<Rule> rules_;
  bool has_request_rules_ = false;
  bool has_response_rules_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_HEADER_RULES_H_
