// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CONTEXT_COUNTER_H_
#define ATOM_COMMON_CONTEXT_COUNTER_H_

namespace atom {

// Increase the context counter, and return current count.
int GetNextContextId();

}  // namespace atom

#endif  // ATOM_COMMON_CONTEXT_COUNTER_H_
