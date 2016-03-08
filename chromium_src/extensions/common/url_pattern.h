// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef EXTENSIONS_COMMON_URL_PATTERN_H_
#define EXTENSIONS_COMMON_URL_PATTERN_H_

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

class GURL;

// A pattern that can be used to match URLs. A URLPattern is a very restricted
// subset of URL syntax:
//
// <url-pattern> := <scheme>://<host><port><path> | '<all_urls>'
// <scheme> := '*' | 'http' | 'https' | 'file' | 'ftp' | 'chrome' |
//             'chrome-extension' | 'filesystem'
// <host> := '*' | '*.' <anychar except '/' and '*'>+
// <port> := [':' ('*' | <port number between 0 and 65535>)]
// <path> := '/' <any chars>
//
// * Host is not used when the scheme is 'file'.
// * The path can have embedded '*' characters which act as glob wildcards.
// * '<all_urls>' is a special pattern that matches any URL that contains a
//   valid scheme (as specified by valid_schemes_).
// * The '*' scheme pattern excludes file URLs.
//
// Examples of valid patterns:
// - http://*/*
// - http://*/foo*
// - https://*.google.com/foo*bar
// - file://monkey*
// - http://127.0.0.1/*
//
// Examples of invalid patterns:
// - http://* -- path not specified
// - http://*foo/bar -- * not allowed as substring of host component
// - http://foo.*.bar/baz -- * must be first component
// - http:/bar -- scheme separator not found
// - foo://* -- invalid scheme
// - chrome:// -- we don't support chrome internal URLs
class URLPattern {
 public:
  // A collection of scheme bitmasks for use with valid_schemes.
  enum SchemeMasks {
    SCHEME_NONE       = 0,
    SCHEME_HTTP       = 1 << 0,
    SCHEME_HTTPS      = 1 << 1,
    SCHEME_FILE       = 1 << 2,
    SCHEME_FTP        = 1 << 3,
    SCHEME_CHROMEUI   = 1 << 4,
    SCHEME_EXTENSION  = 1 << 5,
    SCHEME_FILESYSTEM = 1 << 6,

    // IMPORTANT!
    // SCHEME_ALL will match every scheme, including chrome://, chrome-
    // extension://, about:, etc. Because this has lots of security
    // implications, third-party extensions should usually not be able to get
    // access to URL patterns initialized this way. If there is a reason
    // for violating this general rule, document why this it safe.
    SCHEME_ALL      = -1,
  };

  // Error codes returned from Parse().
  enum ParseResult {
    PARSE_SUCCESS = 0,
    PARSE_ERROR_MISSING_SCHEME_SEPARATOR,
    PARSE_ERROR_INVALID_SCHEME,
    PARSE_ERROR_WRONG_SCHEME_SEPARATOR,
    PARSE_ERROR_EMPTY_HOST,
    PARSE_ERROR_INVALID_HOST_WILDCARD,
    PARSE_ERROR_EMPTY_PATH,
    PARSE_ERROR_INVALID_PORT,
    PARSE_ERROR_INVALID_HOST,
    NUM_PARSE_RESULTS
  };

  // The <all_urls> string pattern.
  static const char kAllUrlsPattern[];

  // Returns true if the given |scheme| is considered valid for extensions.
  static bool IsValidSchemeForExtensions(const std::string& scheme);

  explicit URLPattern(int valid_schemes);

  // Convenience to construct a URLPattern from a string. If the string is not
  // known ahead of time, use Parse() instead, which returns success or failure.
  URLPattern(int valid_schemes, const std::string& pattern);

  URLPattern();
  ~URLPattern();

  bool operator<(const URLPattern& other) const;
  bool operator>(const URLPattern& other) const;
  bool operator==(const URLPattern& other) const;

  // Initializes this instance by parsing the provided string. Returns
  // URLPattern::PARSE_SUCCESS on success, or an error code otherwise. On
  // failure, this instance will have some intermediate values and is in an
  // invalid state.
  ParseResult Parse(const std::string& pattern_str);

  // Gets the bitmask of valid schemes.
  int valid_schemes() const { return valid_schemes_; }
  void SetValidSchemes(int valid_schemes);

  // Gets the host the pattern matches. This can be an empty string if the
  // pattern matches all hosts (the input was <scheme>://*/<whatever>).
  const std::string& host() const { return host_; }
  void SetHost(const std::string& host);

  // Gets whether to match subdomains of host().
  bool match_subdomains() const { return match_subdomains_; }
  void SetMatchSubdomains(bool val);

  // Gets the path the pattern matches with the leading slash. This can have
  // embedded asterisks which are interpreted using glob rules.
  const std::string& path() const { return path_; }
  void SetPath(const std::string& path);

  // Returns true if this pattern matches all urls.
  bool match_all_urls() const { return match_all_urls_; }
  void SetMatchAllURLs(bool val);

  // Sets the scheme for pattern matches. This can be a single '*' if the
  // pattern matches all valid schemes (as defined by the valid_schemes_
  // property). Returns false on failure (if the scheme is not valid).
  bool SetScheme(const std::string& scheme);
  // Note: You should use MatchesScheme() instead of this getter unless you
  // absolutely need the exact scheme. This is exposed for testing.
  const std::string& scheme() const { return scheme_; }

  // Returns true if the specified scheme can be used in this URL pattern, and
  // false otherwise. Uses valid_schemes_ to determine validity.
  bool IsValidScheme(const std::string& scheme) const;

  // Returns true if this instance matches the specified URL.
  bool MatchesURL(const GURL& test) const;

  // Returns true if this instance matches the specified security origin.
  bool MatchesSecurityOrigin(const GURL& test) const;

  // Returns true if |test| matches our scheme.
  // Note that if test is "filesystem", this may fail whereas MatchesURL
  // may succeed.  MatchesURL is smart enough to look at the inner_url instead
  // of the outer "filesystem:" part.
  bool MatchesScheme(const std::string& test) const;

  // Returns true if |test| matches our host.
  bool MatchesHost(const std::string& test) const;
  bool MatchesHost(const GURL& test) const;

  // Returns true if |test| matches our path.
  bool MatchesPath(const std::string& test) const;

  // Returns true if the pattern is vague enough that it implies all hosts,
  // such as *://*/*.
  // This is an expensive method, and should be used sparingly!
  // You should probably use URLPatternSet::ShouldWarnAllHosts(), which is
  // cached.
  bool ImpliesAllHosts() const;

  // Returns true if the pattern only matches a single origin. The pattern may
  // include a path.
  bool MatchesSingleOrigin() const;

  // Sets the port. Returns false if the port is invalid.
  bool SetPort(const std::string& port);
  const std::string& port() const { return port_; }

  // Returns a string representing this instance.
  const std::string& GetAsString() const;

  // Determines whether there is a URL that would match this instance and
  // another instance. This method is symmetrical: Calling
  // other.OverlapsWith(this) would result in the same answer.
  bool OverlapsWith(const URLPattern& other) const;

  // Returns true if this pattern matches all possible URLs that |other| can
  // match. For example, http://*.google.com encompasses http://www.google.com.
  bool Contains(const URLPattern& other) const;

  // Converts this URLPattern into an equivalent set of URLPatterns that don't
  // use a wildcard in the scheme component. If this URLPattern doesn't use a
  // wildcard scheme, then the returned set will contain one element that is
  // equivalent to this instance.
  std::vector<URLPattern> ConvertToExplicitSchemes() const;

  static bool EffectiveHostCompare(const URLPattern& a, const URLPattern& b) {
    if (a.match_all_urls_ && b.match_all_urls_)
      return false;
    return a.host_.compare(b.host_) < 0;
  }

  // Used for origin comparisons in a std::set.
  class EffectiveHostCompareFunctor {
   public:
    bool operator()(const URLPattern& a, const URLPattern& b) const {
      return EffectiveHostCompare(a, b);
    }
  };

  // Get an error string for a ParseResult.
  static const char* GetParseResultString(URLPattern::ParseResult parse_result);

 private:
  // Returns true if any of the |schemes| items matches our scheme.
  bool MatchesAnyScheme(const std::vector<std::string>& schemes) const;

  // Returns true if all of the |schemes| items matches our scheme.
  bool MatchesAllSchemes(const std::vector<std::string>& schemes) const;

  bool MatchesSecurityOriginHelper(const GURL& test) const;

  // Returns true if our port matches the |port| pattern (it may be "*").
  bool MatchesPortPattern(const std::string& port) const;

  // If the URLPattern contains a wildcard scheme, returns a list of
  // equivalent literal schemes, otherwise returns the current scheme.
  std::vector<std::string> GetExplicitSchemes() const;

  // A bitmask containing the schemes which are considered valid for this
  // pattern. Parse() uses this to decide whether a pattern contains a valid
  // scheme.
  int valid_schemes_;

  // True if this is a special-case "<all_urls>" pattern.
  bool match_all_urls_;

  // The scheme for the pattern.
  std::string scheme_;

  // The host without any leading "*" components.
  std::string host_;

  // Whether we should match subdomains of the host. This is true if the first
  // component of the pattern's host was "*".
  bool match_subdomains_;

  // The port.
  std::string port_;

  // The path to match. This is everything after the host of the URL, or
  // everything after the scheme in the case of file:// URLs.
  std::string path_;

  // The path with "?" and "\" characters escaped for use with the
  // MatchPattern() function.
  std::string path_escaped_;

  // A string representing this URLPattern.
  mutable std::string spec_;
};

std::ostream& operator<<(std::ostream& out, const URLPattern& url_pattern);

typedef std::vector<URLPattern> URLPatternList;

#endif  // EXTENSIONS_COMMON_URL_PATTERN_H_
