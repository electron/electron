// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGE_PROPERTY_PROVIDER_H_
#define ATOM_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGE_PROPERTY_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"

class GURL;

namespace base {
class TaskRunner;
}

namespace content {
class BrowserContext;
}

namespace net {
class URLRequestContextGetter;
}

namespace extensions {

// This class provides properties of messages asynchronously.
class MessagePropertyProvider {
 public:
  MessagePropertyProvider();

  typedef base::Callback<void(const std::string&)> ChannelIDCallback;

  // Gets the DER-encoded public key of the domain-bound cert,
  // aka TLS channel ID, for the given URL.
  // Runs |reply| on the current message loop.
  void GetChannelID(content::BrowserContext* browser_context,
                    const GURL& source_url,
                    const ChannelIDCallback& reply);

 private:
  struct GetChannelIDOutput;

  static void GetChannelIDOnIOThread(
      scoped_refptr<base::TaskRunner> original_task_runner,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      const std::string& host,
      const ChannelIDCallback& reply);

  static void GotChannelID(
      scoped_refptr<base::TaskRunner> original_task_runner,
      struct GetChannelIDOutput* output,
      const ChannelIDCallback& reply,
      int status);

  DISALLOW_COPY_AND_ASSIGN(MessagePropertyProvider);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_API_MESSAGING_MESSAGE_PROPERTY_PROVIDER_H_
