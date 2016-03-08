// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SIMPLE_MESSAGE_BOX_H_
#define CHROME_BROWSER_UI_SIMPLE_MESSAGE_BOX_H_

#include "base/strings/string16.h"
#include "ui/gfx/native_widget_types.h"

namespace chrome {

enum MessageBoxResult {
  MESSAGE_BOX_RESULT_NO = 0,  // User chose NO or CANCEL.
  MESSAGE_BOX_RESULT_YES = 1, // User chose YES or OK.
};

enum MessageBoxType {
  MESSAGE_BOX_TYPE_INFORMATION,  // Shows an OK button.
  MESSAGE_BOX_TYPE_WARNING,      // Shows an OK button.
  MESSAGE_BOX_TYPE_QUESTION,     // Shows YES and NO buttons.
  MESSAGE_BOX_TYPE_OK_CANCEL,    // Shows OK and CANCEL buttons (Aura only).
};

// Shows a dialog box with the given |title| and |message|. If |parent| is
// non-NULL, the box will be made modal to the |parent|, except on Mac, where it
// is always app-modal.
//
// NOTE: In general, you should avoid this since it's usually poor UI.
// We have a variety of other surfaces such as app menu notifications and
// infobars; consult the UI leads for a recommendation.
MessageBoxResult ShowMessageBox(gfx::NativeWindow parent,
                                const base::string16& title,
                                const base::string16& message,
                                MessageBoxType type);

// Shows a dialog box with the given |title| and |message|, and with two buttons
// labeled with |yes_text| and |no_text|. If |parent| is non-NULL, the box will
// be made modal to the |parent|.  (Aura only.)
//
// NOTE: In general, you should avoid this since it's usually poor UI.
// We have a variety of other surfaces such as app menu notifications and
// infobars; consult the UI leads for a recommendation.
MessageBoxResult ShowMessageBoxWithButtonText(gfx::NativeWindow parent,
                                              const base::string16& title,
                                              const base::string16& message,
                                              const base::string16& yes_text,
                                              const base::string16& no_text);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_SIMPLE_MESSAGE_BOX_H_
