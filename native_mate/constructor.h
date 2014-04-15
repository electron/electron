// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_WRAPPABLE_CLASS_H_
#define NATIVE_MATE_WRAPPABLE_CLASS_H_

#include "native_mate/wrappable.h"

namespace mate {

class Constructor {
 public:
  v8::Handle<v8::Function> GetFunction(v8::Isolate* isolate);

 protected:
  Constructor(const base::StringPiece& name);
  virtual ~Constructor();

  virtual void New() = 0;

  virtual void SetPrototype(v8::Isolate* isolate,
                            v8::Handle<v8::ObjectTemplate> prototype);

 private:
  base::StringPiece name_;
  v8::Persistent<v8::FunctionTemplate> constructor_;

  DISALLOW_COPY_AND_ASSIGN(Constructor);
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_CLASS_H_
