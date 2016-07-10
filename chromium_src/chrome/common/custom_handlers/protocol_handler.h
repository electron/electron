// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CUSTOM_HANDLERS_PROTOCOL_HANDLER_H_
#define CHROME_COMMON_CUSTOM_HANDLERS_PROTOCOL_HANDLER_H_

#include <string>

#include "base/values.h"
#include "url/gurl.h"

// A single tuple of (protocol, url) that indicates how URLs of the
// given protocol should be rewritten to be handled.

class ProtocolHandler {
 public:
  static ProtocolHandler CreateProtocolHandler(const std::string& protocol,
                                               const GURL& url);

  // Creates a ProtocolHandler with fields from the dictionary. Returns an
  // empty ProtocolHandler if the input is invalid.
  static ProtocolHandler CreateProtocolHandler(
      const base::DictionaryValue* value);

  // Returns true if the dictionary value has all the necessary fields to
  // define a ProtocolHandler.
  static bool IsValidDict(const base::DictionaryValue* value);

  // Returns true if this handler's url has the same origin as the given one.
  bool IsSameOrigin(const ProtocolHandler& handler) const;

  // Canonical empty ProtocolHandler.
  static const ProtocolHandler& EmptyProtocolHandler();

  // Interpolates the given URL into the URL template of this handler.
  GURL TranslateUrl(const GURL& url) const;

  // Returns true if the handlers are considered equivalent when determining
  // if both handlers can be registered, or if a handler has previously been
  // ignored.
  bool IsEquivalent(const ProtocolHandler& other) const;

  // Encodes this protocol handler as a DictionaryValue. The caller is
  // responsible for deleting the returned value.
  base::DictionaryValue* Encode() const;

  const std::string& protocol() const { return protocol_; }
  const GURL& url() const { return url_;}

  bool IsEmpty() const {
    return protocol_.empty();
  }

#if !defined(NDEBUG)
  // Returns a string representation suitable for use in debugging.
  std::string ToString() const;
#endif


  bool operator==(const ProtocolHandler& other) const;
  bool operator<(const ProtocolHandler& other) const;

 private:
  ProtocolHandler(const std::string& protocol,
                  const GURL& url);
  ProtocolHandler();

  std::string protocol_;
  GURL url_;
};

#endif  // CHROME_COMMON_CUSTOM_HANDLERS_PROTOCOL_HANDLER_H_

