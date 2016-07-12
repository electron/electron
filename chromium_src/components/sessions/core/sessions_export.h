// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_SESSIONS_EXPORT_H_
#define COMPONENTS_SESSIONS_CORE_SESSIONS_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(SESSIONS_IMPLEMENTATION)
#define SESSIONS_EXPORT __declspec(dllexport)
#else
#define SESSIONS_EXPORT __declspec(dllimport)
#endif  // defined(SESSIONS_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(SESSIONS_IMPLEMENTATION)
#define SESSIONS_EXPORT __attribute__((visibility("default")))
#else
#define SESSIONS_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define SESSIONS_EXPORT
#endif

#endif  // COMPONENTS_SESSIONS_CORE_SESSIONS_EXPORT_H_
