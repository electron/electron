// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ATOM_MESSAGE_FILTER_H_
#define SHELL_BROWSER_ATOM_MESSAGE_FILTER_H_

#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/web_contents.h"

namespace electron {

class AtomMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit AtomMessageFilter(content::WebContents* web_contents);

  // content::BrowserMessageFilter implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~AtomMessageFilter() override;

  void ForwardMessage(const IPC::Message& message);

  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(AtomMessageFilter);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ATOM_MESSAGE_FILTER_H_
