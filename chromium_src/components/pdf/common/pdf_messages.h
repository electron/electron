// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.
#include <string.h>

#include "content/public/common/common_param_traits_macros.h"
#include "content/public/common/referrer.h"
#include "ipc/ipc_message_macros.h"
#include "url/gurl.h"
#include "url/ipc/url_param_traits.h"

#define IPC_MESSAGE_START PDFMsgStart

// Updates the content restrictions, i.e. to disable print/copy.
IPC_MESSAGE_ROUTED1(PDFHostMsg_PDFUpdateContentRestrictions,
                    int /* restrictions */)

// The currently displayed PDF has an unsupported feature.
IPC_MESSAGE_ROUTED0(PDFHostMsg_PDFHasUnsupportedFeature)

// Brings up SaveAs... dialog to save specified URL.
IPC_MESSAGE_ROUTED2(PDFHostMsg_PDFSaveURLAs,
                    GURL /* url */,
                    content::Referrer /* referrer */)
