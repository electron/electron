// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_TRY_CATCH_H_
#define NATIVE_MATE_TRY_CATCH_H_

#include <string>

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace mate {

// TryCatch is a convenient wrapper around v8::TryCatch.
class TryCatch {
 public:
  TryCatch();
  ~TryCatch();

  bool HasCaught();
  std::string GetStackTrace();

 private:
  v8::TryCatch try_catch_;

  DISALLOW_COPY_AND_ASSIGN(TryCatch);
};

}  // namespace mate

#endif  // NATIVE_MATE_TRY_CATCH_H_
