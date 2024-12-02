// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_

#include <string>

#include "content/public/browser/frame_tree_node_id.h"
#include "third_party/blink/public/mojom/loader/transferrable_url_loader.mojom-forward.h"

class GURL;

namespace extensions {

// TODO(devlin): This is now only used for the MimeTypesHandler API. We should
// rename and move it to make that clear. https://crbug.com/890401.
class StreamsPrivateAPI {
 public:
  // Send the onExecuteMimeTypeHandler event to |extension_id|. A non-empty
  // |stream_id| will be used to identify the created stream during
  // MimeHandlerViewGuest creation. |embedded| should be set to whether the
  // document is embedded within another document. The |frame_tree_node_id|
  // parameter is used for the top level plugins case. (PDF, etc).
  static void SendExecuteMimeTypeHandlerEvent(
      const std::string& extension_id,
      const std::string& stream_id,
      bool embedded,
      content::FrameTreeNodeId frame_tree_node_id,
      blink::mojom::TransferrableURLLoaderPtr transferrable_loader,
      const GURL& original_url,
      const std::string& internal_id);
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
