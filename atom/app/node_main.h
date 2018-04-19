// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_NODE_MAIN_H_
#define ATOM_APP_NODE_MAIN_H_

#ifdef ENABLE_RUN_AS_NODE

namespace atom {

int NodeMain(int argc, char* argv[]);

}  // namespace atom

#endif  // ENABLE_RUN_AS_NODE

#endif  // ATOM_APP_NODE_MAIN_H_
