// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_DIALOG_H_
#define ATOM_BROWSER_API_ATOM_API_DIALOG_H_

#include "v8/include/v8.h"

namespace atom {

namespace api {

v8::Handle<v8::Value> ShowMessageBox(const v8::Arguments &args);
v8::Handle<v8::Value> ShowOpenDialog(const v8::Arguments &args);
v8::Handle<v8::Value> ShowSaveDialog(const v8::Arguments &args);

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_DIALOG_H_
