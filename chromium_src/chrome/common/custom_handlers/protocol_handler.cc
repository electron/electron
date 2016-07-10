// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/custom_handlers/protocol_handler.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/escape.h"


ProtocolHandler::ProtocolHandler(const std::string& protocol,
                                 const GURL& url)
    : protocol_(protocol),
      url_(url) {
}

ProtocolHandler ProtocolHandler::CreateProtocolHandler(
    const std::string& protocol,
    const GURL& url) {
  std::string lower_protocol = base::ToLowerASCII(protocol);
  return ProtocolHandler(lower_protocol, url);
}

ProtocolHandler::ProtocolHandler() {
}

bool ProtocolHandler::IsValidDict(const base::DictionaryValue* value) {
  // Note that "title" parameter is ignored.
  return value->HasKey("protocol") && value->HasKey("url");
}

bool ProtocolHandler::IsSameOrigin(
    const ProtocolHandler& handler) const {
  return handler.url().GetOrigin() == url_.GetOrigin();
}

const ProtocolHandler& ProtocolHandler::EmptyProtocolHandler() {
  static const ProtocolHandler* const kEmpty = new ProtocolHandler();
  return *kEmpty;
}

ProtocolHandler ProtocolHandler::CreateProtocolHandler(
    const base::DictionaryValue* value) {
  if (!IsValidDict(value)) {
    return EmptyProtocolHandler();
  }
  std::string protocol, url;
  value->GetString("protocol", &protocol);
  value->GetString("url", &url);
  return ProtocolHandler::CreateProtocolHandler(protocol, GURL(url));
}

GURL ProtocolHandler::TranslateUrl(const GURL& url) const {
  std::string translatedUrlSpec(url_.spec());
  base::ReplaceSubstringsAfterOffset(&translatedUrlSpec, 0, "%s",
      net::EscapeQueryParamValue(url.spec(), true));
  return GURL(translatedUrlSpec);
}

base::DictionaryValue* ProtocolHandler::Encode() const {
  base::DictionaryValue* d = new base::DictionaryValue();
  d->Set("protocol", new base::StringValue(protocol_));
  d->Set("url", new base::StringValue(url_.spec()));
  return d;
}

#if !defined(NDEBUG)
std::string ProtocolHandler::ToString() const {
  return "{ protocol=" + protocol_ +
         ", url=" + url_.spec() +
         " }";
}
#endif

bool ProtocolHandler::operator==(const ProtocolHandler& other) const {
  return protocol_ == other.protocol_ && url_ == other.url_;
}

bool ProtocolHandler::IsEquivalent(const ProtocolHandler& other) const {
  return protocol_ == other.protocol_ && url_ == other.url_;
}

bool ProtocolHandler::operator<(const ProtocolHandler& other) const {
  return url_ < other.url_;
}
