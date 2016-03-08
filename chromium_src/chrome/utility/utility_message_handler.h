// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_UTILITY_UTILITY_MESSAGE_HANDLER_H_
#define CHROME_UTILITY_UTILITY_MESSAGE_HANDLER_H_

namespace IPC {
class Message;
}

class UtilityMessageHandler {
 public:
  virtual ~UtilityMessageHandler() {}

  // Called when a message is received.  Returns true iff the message was
  // handled.
  virtual bool OnMessageReceived(const IPC::Message& message) = 0;
};

#endif  // CHROME_UTILITY_UTILITY_MESSAGE_HANDLER_H_

