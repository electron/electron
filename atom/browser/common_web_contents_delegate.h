// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
#define ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_

#include "brightray/browser/default_web_contents_delegate.h"

namespace atom {

class CommonWebContentsDelegate : public brightray::DefaultWebContentsDelegate {
 public:
  explicit CommonWebContentsDelegate(bool is_guest);
  virtual ~CommonWebContentsDelegate();

  bool is_guest() const { return is_guest_; };

 private:
  const bool is_guest_;

  DISALLOW_COPY_AND_ASSIGN(CommonWebContentsDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
