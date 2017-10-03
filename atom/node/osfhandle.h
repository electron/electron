// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_NODE_OSFHANDLE_H_
#define ATOM_NODE_OSFHANDLE_H_

namespace node {

// A trick to force referencing symbols.
__declspec(dllexport) void ReferenceSymbols();

}  // namespace node

#endif  // ATOM_NODE_OSFHANDLE_H_
