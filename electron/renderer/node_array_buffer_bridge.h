// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_RENDERER_NODE_ARRAY_BUFFER_BRIDGE_H_
#define ELECTRON_RENDERER_NODE_ARRAY_BUFFER_BRIDGE_H_

namespace atom {

// Override Node's ArrayBuffer with DOM's ArrayBuffer.
void OverrideNodeArrayBuffer();

}  // namespace atom

#endif  // ELECTRON_RENDERER_NODE_ARRAY_BUFFER_BRIDGE_H_
