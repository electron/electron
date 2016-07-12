// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_SESSION_ID_H_
#define COMPONENTS_SESSIONS_CORE_SESSION_ID_H_

#include <stdint.h>

#include "components/sessions/core/sessions_export.h"

// Uniquely identifies a tab or window for the duration of a session.
class SESSIONS_EXPORT SessionID {
 public:
  typedef int32_t id_type;

  SessionID();
  ~SessionID() {}

  // Returns the underlying id.
  void set_id(id_type id) { id_ = id; }
  id_type id() const { return id_; }

 private:
  id_type id_;
};

#endif  // COMPONENTS_SESSIONS_CORE_SESSION_ID_H_
