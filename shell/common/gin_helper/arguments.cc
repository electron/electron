// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/arguments.h"

#include "v8/include/v8-exception.h"
namespace gin_helper {

void Arguments::ThrowError() const {
  // Gin advances |next_| counter when conversion fails while we do not, so we
  // have to manually advance the counter here to make gin report error with the
  // correct index.
  const_cast<Arguments*>(this)->Skip();
  gin::Arguments::ThrowError();
}

void Arguments::ThrowError(base::StringPiece message) const {
  isolate()->ThrowException(
      v8::Exception::Error(gin::StringToV8(isolate(), message)));
}

}  // namespace gin_helper
