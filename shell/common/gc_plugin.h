// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GC_PLUGIN_H_
#define ELECTRON_SHELL_COMMON_GC_PLUGIN_H_

// GC_PLUGIN_IGNORE is used to make the Blink GC plugin ignore a particular
// class or field when checking for proper usage. When using GC_PLUGIN_IGNORE
// a reason must be provided as an argument.
// Based on third_party/blink/renderer/platform/wtf/gc_plugin.h
#if defined(__clang__)
#define GC_PLUGIN_IGNORE(reason) \
  __attribute__((annotate("blink_gc_plugin_ignore")))
#else
#define GC_PLUGIN_IGNORE(reason)
#endif

#endif  // ELECTRON_SHELL_COMMON_GC_PLUGIN_H_
