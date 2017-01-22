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

// Brings up SaveAs... dialog to save specified URL.
IPC_MESSAGE_ROUTED2(PDFHostMsg_PDFSaveURLAs,
                    GURL /* url */,
                    content::Referrer /* referrer */)
